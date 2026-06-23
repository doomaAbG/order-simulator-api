#include "order_publisher.hpp"
#include <iostream>

OrderPublisher::OrderPublisher(const std::string& brokers) {
    std::string errstr;
    RdKafka::Conf* conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    
    if (conf->set("bootstrap.servers", brokers, errstr) != RdKafka::Conf::CONF_OK) {
        std::cerr << "Kafka Conf Error: " << errstr << std::endl;
        delete conf;
        throw std::runtime_error("Failed to set Kafka brokers");
    }

    m_producer = RdKafka::Producer::create(conf, errstr);
    if (!m_producer) {
        std::cerr << "Failed to create Kafka producer: " << errstr << std::endl;
        delete conf;
        throw std::runtime_error("Failed to create Kafka producer");
    }
    
    delete conf; // Очищаем конфиг, он больше не нужен
}

OrderPublisher::~OrderPublisher() {
    if (m_producer) {
        delete m_producer;
    }
}

void OrderPublisher::publish_order(int order_id, double total_price) {
    // Формируем JSON-строку сообщения
    std::string kafka_msg = "{\"order_id\":" + std::to_string(order_id) + 
                            ",\"status\":\"PENDING\",\"total_price\":" + std::to_string(total_price) + "}";

    // Публикуем в топик "order_topic"
    RdKafka::ErrorCode resp = m_producer->produce(
        "order_topic",
        RdKafka::Topic::PARTITION_UA,
        RdKafka::Producer::RK_MSG_COPY,
        const_cast<char*>(kafka_msg.c_str()), kafka_msg.size(),
        NULL, 0, 0, NULL, NULL
    );

    if (resp != RdKafka::ERR_NO_ERROR) {
        std::cerr << "Kafka produce failed: " << RdKafka::err2str(resp) << std::endl;
    } else {
        std::cout << "Successfully sent order " << order_id << " to Kafka topic!" << std::endl;
    }
    
    // Проталкиваем буфер
    m_producer->poll(0);
}