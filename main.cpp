#include <crow.h>
#include <pqxx/pqxx>
#include <librdkafka/rdkafkacpp.h>
#include <iostream>
#include <string>

const std::string CONN_STR = "host=127.0.0.1 port=5432 dbname=order_simulator user=postgres password=0000";

int main() {
    crow::SimpleApp app;

    // --- НАСТРОЙКА KAFKA ---
    std::string errstr;
    RdKafka::Conf* conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    
    // Указываем адрес брокера (если Кафка в докере на этой же машине, то localhost:9092)
    if (conf->set("bootstrap.servers", "localhost:9092", errstr) != RdKafka::Conf::CONF_OK) {
        std::cerr << "Kafka Conf Error: " << errstr << std::endl;
        return 1;
    }

    // Создаем продюсера
    RdKafka::Producer* producer = RdKafka::Producer::create(conf, errstr);
    if (!producer) {
        std::cerr << "Failed to create Kafka producer: " << errstr << std::endl;
        return 1;
    }
    delete conf; // Конфиг больше не нужен, можно удалить
    // -----------------------

    CROW_ROUTE(app, "/orders").methods(crow::HTTPMethod::POST)([producer](const crow::request& req) {
        try {
            auto json_data = crow::json::load(req.body);
            if (!json_data) {
                return crow::response(400, "Invalid JSON");
            }

            int product_id = json_data["product_id"].i();
            int quantity = json_data["quantity"].i();
            double total_price = json_data["total_price"].d();

            // 1. Сохраняем в PostgreSQL
            pqxx::connection conn(CONN_STR);
            pqxx::work tx(conn);

            std::string query = "INSERT INTO orders (product_id, quantity, total_price, status) VALUES (" 
                                + std::to_string(product_id) + ", " 
                                + std::to_string(quantity) + ", " 
                                + std::to_string(total_price) + ", 'PENDING') RETURNING id";

            pqxx::result res = tx.exec(query);
            int order_id = res[0][0].as<int>();
            tx.commit();

            // 2. Формируем сообщение для Кафки
            std::string kafka_msg = "{\"order_id\":" + std::to_string(order_id) + 
                                    ",\"status\":\"PENDING\",\"total_price\":" + std::to_string(total_price) + "}";

            // 3. Публикуем в топик "order_topic"
            RdKafka::ErrorCode resp = producer->produce(
                "order_topic",             // Топик
                RdKafka::Topic::PARTITION_UA, // Авто-выбор партиции
                RdKafka::Producer::RK_MSG_COPY, // Копировать данные из строки
                const_cast<char*>(kafka_msg.c_str()), kafka_msg.size(),
                NULL, 0, 0, NULL, NULL
            );

            if (resp != RdKafka::ERR_NO_ERROR) {
                std::cerr << "Kafka produce failed: " << RdKafka::err2str(resp) << std::endl;
            } else {
                std::cout << "Successfully sent order " << order_id << " to Kafka topic!" << std::endl;
            }
            
            // Быстро проталкиваем буфер сообщений
            producer->poll(0);

            // Отвечаем клиенту
            crow::json::wvalue response;
            response["status"] = "Order submitted and sent to Kafka";
            response["order_id"] = order_id;
            
            return crow::response(201, response);

        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return crow::response(500, "Internal Server Error");
        }
    });

    app.port(8080).multithreaded().run();

    // Очистка ресурсов перед выходом
    delete producer;
    return 0;
}