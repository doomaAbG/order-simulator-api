#pragma once
#include <string>
#include <librdkafka/rdkafkacpp.h>

class OrderPublisher {
public:
    // Конструктор инициализирует продюсера Кафки
    explicit OrderPublisher(const std::string& brokers);
    
    // Деструктор аккуратно подчищает память за Кафкой
    ~OrderPublisher();

    // Метод для отправки сообщения о новом заказе
    void publish_order(int order_id, double total_price);

private:
    RdKafka::Producer* m_producer;
};