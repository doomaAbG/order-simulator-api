#pragma once
#include <string>
#include <pqxx/pqxx>

class OrderRepository {
public:
    // Конструктор принимает строку подключения к БД
    explicit OrderRepository(const std::string& conn_str);

    // Метод сохраняет заказ и возвращает его сгенерированный ID
    int save_order(int product_id, int quantity, double total_price);

private:
    std::string m_conn_str;
};