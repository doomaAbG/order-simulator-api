#include "order_repo.hpp"
#include <iostream>

OrderRepository::OrderRepository(const std::string& conn_str) 
    : m_conn_str(conn_str) {}

int OrderRepository::save_order(int product_id, int quantity, double total_price) {
    // Открываем соединение и стартуем транзакцию
    pqxx::connection conn(m_conn_str);
    pqxx::work tx(conn);

    // Безопасный запрос с плейсхолдерами $1, $2, $3 вместо ручной склейки строк
    std::string query = "INSERT INTO orders (product_id, quantity, total_price, status) "
                        "VALUES ($1, $2, $3, 'PENDING') RETURNING id";
    
    // Выполняем запрос, передавая параметры отдельно
    pqxx::result res = tx.exec_params(query, product_id, quantity, total_price);
    
    // Забираем ID
    int order_id = res[0][0].as<int>();
    
    // Фиксируем транзакцию
    tx.commit();
    
    return order_id;
}