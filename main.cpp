#include <crow.h>
#include <string>
#include <iostream>

// Подключаем наши собственные архитектурные слои
#include "repository/order_repo.hpp"
#include "messaging/order_publisher.hpp"

// Константы конфигурации
const std::string CONN_STR = "host=127.0.0.1 port=5432 dbname=order_simulator user=postgres password=0000";
const std::string KAFKA_BROKERS = "localhost:9092";

int main() {
    crow::SimpleApp app;

    try {
        // Инициализируем наши слои один раз при старте сервера
        // Они будут жить на протяжении всей работы программы
        OrderRepository repo(CONN_STR);
        OrderPublisher publisher(KAFKA_BROKERS);

        // HTTP ручка для создания заказов
        // Мы захватываем ссылки на репозиторий и издатель [&repo, &publisher]
        CROW_ROUTE(app, "/orders").methods("POST"_method)([&repo, &publisher](const crow::request& req) {
            try {
                auto json_data = crow::json::load(req.body);
                if (!json_data) {
                    return crow::response(400, "Invalid JSON");
                }

                // Извлекаем данные из HTTP-запроса
                int product_id = json_data["product_id"].i();
                int quantity = json_data["quantity"].i();
                double total_price = json_data["total_price"].d();

                // ИСПОЛЬЗУЕМ СЛОЙ ДАННЫХ: Сохраняем в PostgreSQL через репозиторий
                int order_id = repo.save_order(product_id, quantity, total_price);

                // ИСПОЛЬЗУЕМ СЛОЙ ОЧЕРЕДЕЙ: Отправляем событие в Кафку
                publisher.publish_order(order_id, total_price);

                // Формируем красивый JSON ответ клиенту
                crow::json::wvalue response;
                response["status"] = "Order submitted successfully (Clean Architecture)";
                response["order_id"] = order_id;
                
                return crow::response(201, response);

            } catch (const std::exception& e) {
                std::cerr << "Handler Error: " << e.what() << std::endl;
                return crow::response(500, "Internal Server Error");
            }
        });

        // Запуск сервера в многопоточном режиме
        app.port(8080).multithreaded().run();

    } catch (const std::exception& e) {
        std::cerr << "Fatal Critical Error on Start: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}