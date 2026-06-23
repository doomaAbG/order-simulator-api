import pytest
import requests

# Базовый URL нашего запущенного C++ сервера
BASE_URL = "http://localhost:8080"

def test_create_order_success():
    """Позитивный тест: проверяем успешное создание заказа для товара №42"""
    payload = {
        "product_id": 42,
        "quantity": 2,
        "total_price": 399.98
    }
    
    # Отправляем POST-запрос
    response = requests.post(f"{BASE_URL}/orders", json=payload)
    
    # Проверяем, что сервер вернул статус 201 Created
    assert response.status_code == 201
    
    # Проверяем структуру JSON-ответа
    data = response.json()
    assert "order_id" in data
    assert "status" in data
    assert data["order_id"] > 0
    print(f"\n[SUCCESS] Создан заказ с ID: {data['order_id']}")


def test_create_order_non_existent_product():
    """Негативный тест: база должна выкинуть 500 (или 400), если товара нет"""
    payload = {
        "product_id": 99999,  # Такого товара точно нет в базе
        "quantity": 1,
        "total_price": 10.00
    }
    
    response = requests.post(f"{BASE_URL}/orders", json=payload)
    
    # Наш сервер пока возвращает 500 при нарушении Foreign Key
    assert response.status_code == 500
    print("\n[SUCCESS] Сервер корректно отклонил несуществующий продукт")


def test_create_order_invalid_json():
    """Негативный тест: проверяем реакцию сервера на битый JSON"""
    bad_data = "{ 'product_id': 42, 'quantity': "  # Сломанный синтаксис
    
    headers = {'Content-Type': 'application/json'}
    response = requests.post(f"{BASE_URL}/orders", data=bad_data, headers=headers)
    
    # Crow должен вернуть 400 Bad Request (у нас прописано в коде "Invalid JSON")
    assert response.status_code == 400
    print("\n[SUCCESS] Сервер корректно поймал битый JSON")