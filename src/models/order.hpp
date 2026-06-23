#pragma once
#include <string>

struct Order {
    int id;
    int product_id;
    int quantity;
    double total_price;
    std::string status;
};