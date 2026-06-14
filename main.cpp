#define PQXX_SHARED 

#include <crow.h>
#include <pqxx/pqxx>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <windows.h> 

d
using namespace crow;

void asyncOrderProcessor(std::string conn_str) {
    while (true) {
        try {
            pqxx::connection c(conn_str);
            pqxx::work tx(c);

            // Ищем самый старый заказ со статусом PENDING
            pqxx::result res = tx.exec_params(
                "SELECT id FROM orders WHERE status = 'PENDING' ORDER BY id ASC LIMIT 1 FOR UPDATE SKIP LOCKED"
            );

            if (!res.empty()) {
                int order_id = res[0]["id"].as<int>();
                
                
                tx.exec_params("UPDATE orders SET status = 'PROCESSING' WHERE id = $1", order_id);
                tx.commit();
                
                std::cout << "\n[Worker Thread] >>> Заказ #" << order_id << " принят в работу и отправлен на кухню!" << std::endl;

                
                std::this_thread::sleep_for(std::chrono::seconds(5));

                
                pqxx::work tx_complete(c);
                tx_complete.exec_params("UPDATE orders SET status = 'COMPLETED' WHERE id = $1", order_id);
                tx_complete.commit();
                
                std::cout << "[Worker Thread] <<< Заказ #" << order_id << " полностью ГОТОВ и выдан клиенту!\n" << std::endl;
            } else {
                tx.commit();
            }
        } 
        catch (const std::exception& e) {
            std::cerr << "[Worker Error] Ошибка в фоновом потоке: " << e.what() << std::endl;
        }

        
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}


int main() {
    
    SetConsoleOutputCP(65001); 
    SetConsoleCP(65001);

    crow::SimpleApp app;

    
    std::string conn_str = "host=localhost port=5432 dbname=order_simulator user=postgres password=0000";

    
    CROW_ROUTE(app, "/")([](){
        return "Welcome to the Order Simulator API (PostgreSQL Edition)!";
    });

    
    CROW_ROUTE(app, "/products").methods("GET"_method)([&conn_str](){
        crow::json::wvalue response;
        try {
            pqxx::connection c(conn_str);
            pqxx::work tx(c);
            
            pqxx::result res = tx.exec("SELECT id, name, price, stock FROM products ORDER BY id ASC");
            
            std::vector<crow::json::wvalue> products_list;
            for (auto row : res) {
                crow::json::wvalue prod;
                prod["id"] = row["id"].as<int>();
                prod["name"] = row["name"].as<std::string>();
                prod["price"] = row["price"].as<double>();
                prod["stock"] = row["stock"].as<int>();
                products_list.push_back(prod);
            }
            
            tx.commit();
            return crow::response(200, crow::json::wvalue(products_list));
        } 
        catch (const std::exception& e) {
            response["error"] = "Database error: " + std::string(e.what());
            return crow::response(500, response);
        }
    });

    
    CROW_ROUTE(app, "/orders").methods("POST"_method)([&conn_str](const crow::request& req) {
        auto x = crow::json::load(req.body);
        crow::json::wvalue response;

        if (!x || !x.has("product_id") || !x.has("quantity")) {
            response["error"] = "Invalid JSON. Required: product_id, quantity";
            return crow::response(400, response);
        }

        int product_id = x["product_id"].i();
        int quantity = x["quantity"].i();

        if (quantity <= 0) {
            response["error"] = "Quantity must be greater than 0";
            return crow::response(400, response);
        }

        try {
            pqxx::connection c(conn_str);
            pqxx::work tx(c);

            pqxx::result res = tx.exec_params(
                "SELECT price, stock FROM products WHERE id = $1 FOR UPDATE", product_id
            );

            if (res.empty()) {
                response["error"] = "Product not found";
                return crow::response(404, response);
            }

            double price = res[0]["price"].as<double>();
            int current_stock = res[0]["stock"].as<int>();

            if (current_stock < quantity) {
                response["error"] = "Not enough stock. Available: " + std::to_string(current_stock);
                return crow::response(400, response);
            }

            double total_price = price * quantity;

            
            tx.exec_params("UPDATE products SET stock = stock - $1 WHERE id = $2", quantity, product_id);

            
            tx.exec_params(
                "INSERT INTO orders (product_id, quantity, total_price, status) VALUES ($1, $2, $3, $4)",
                product_id, quantity, total_price, "PENDING"
            );

            tx.commit();

            response["status"] = "Success";
            response["message"] = "Order submitted! Processing in background...";
            response["total_price"] = total_price;
            return crow::response(201, response);

        } catch (const std::exception& e) {
            response["error"] = "Database error: " + std::string(e.what());
            return crow::response(500, response);
        }
    });


    CROW_ROUTE(app, "/orders-history").methods("GET"_method)([&conn_str](){
        crow::json::wvalue response;
        try {
            pqxx::connection c(conn_str);
            pqxx::work tx(c);
            
            std::string query = 
                "SELECT o.id, p.name AS product_name, o.quantity, o.total_price, o.status "
                "FROM orders o "
                "INNER JOIN products p ON o.product_id = p.id "
                "ORDER BY o.id DESC";
                
            pqxx::result res = tx.exec(query);
            
            std::vector<crow::json::wvalue> orders_list;
            for (auto row : res) {
                crow::json::wvalue ord;
                ord["order_id"] = row["id"].as<int>();
                ord["product_name"] = row["product_name"].as<std::string>();
                ord["quantity"] = row["quantity"].as<int>();
                ord["total_price"] = row["total_price"].as<double>();
                ord["status"] = row["status"].as<std::string>();
                orders_list.push_back(ord);
            }
            
            tx.commit();
            return crow::response(200, crow::json::wvalue(orders_list));
        } 
        catch (const std::exception& e) {
            response["error"] = "Database error: " + std::string(e.what());
            return crow::response(500, response);
        }
    });

   
    CROW_ROUTE(app, "/orders/status").methods("PATCH"_method)([&conn_str](const crow::request& req) {
        auto x = crow::json::load(req.body);
        crow::json::wvalue response;

        if (!x || !x.has("order_id") || !x.has("status")) {
            response["error"] = "Invalid JSON. Required: order_id, status";
            return crow::response(400, response);
        }

        int order_id = x["order_id"].i();
        std::string new_status = x["status"].s();

        if (new_status != "PENDING" && new_status != "PROCESSING" && new_status != "COMPLETED" && new_status != "CANCELLED") {
            response["error"] = "Invalid status. Choose from: PENDING, PROCESSING, COMPLETED, CANCELLED";
            return crow::response(400, response);
        }

        try {
            pqxx::connection c(conn_str);
            pqxx::work tx(c);

            pqxx::result check_res = tx.exec_params("SELECT id FROM orders WHERE id = $1", order_id);
            if (check_res.empty()) {
                response["error"] = "Order not found";
                return crow::response(404, response);
            }

            tx.exec_params("UPDATE orders SET status = $1 WHERE id = $2", new_status, order_id);
            tx.commit();

            response["status"] = "Success";
            response["message"] = "Order status updated manually to " + new_status;
            return crow::response(200, response);

        } catch (const std::exception& e) {
            response["error"] = "Database error: " + std::string(e.what());
            return crow::response(500, response);
        }
    });

    std::thread worker(asyncOrderProcessor, conn_str);
    worker.detach(); 

    std::cout << "Order Simulator with Async Multithreading Worker is running on port 18080..." << std::endl;
    

    app.port(18080).multithreaded().run();
}