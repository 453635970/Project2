
#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <string>
#include <memory>

// 订单类型枚举
enum class OrderType {
    BUY,  // 买单
    SELL  // 卖单
};

// 订单结构
struct Order {
    int id;              // 订单ID
    OrderType type;      // 订单类型
    double price;        // 价格
    int quantity;        // 数量
    int timestamp;       // 时间戳
    
    Order(int id, OrderType type, double price, int quantity, int timestamp)
        : id(id), type(type), price(price), quantity(quantity), timestamp(timestamp) {}
};

// 成交记录
struct Trade {
    int buyOrderId;      // 买单ID
    int sellOrderId;     // 卖单ID
    double price;        // 成交价
    int quantity;        // 成交数量
    int timestamp;       // 成交时间
    
    Trade(int buyId, int sellId, double price, int qty, int time)
        : buyOrderId(buyId), sellOrderId(sellId), price(price), quantity(qty), timestamp(time) {}
};

class MatchingEngine
 {
private:
    // 使用优先队列实现买卖盘
    // 买单按价格降序排列（价格高的优先）
    std::priority_queue<std::shared_ptr<Order>, 
                        std::vector<std::shared_ptr<Order>>, 
                       bool(*)(const std::shared_ptr<Order>&, const std::shared_ptr<Order>&)> buyOrders;
    
    // 卖单按价格升序排列（价格低的优先）
    std::priority_queue<std::shared_ptr<Order>, 
                        std::vector<std::shared_ptr<Order>>, 
                       bool(*)(const std::shared_ptr<Order>&, const std::shared_ptr<Order>&)> sellOrders;
    
    std::vector<Trade> tradeHistory;  // 成交历史
    int nextOrderId;                 // 下一个订单ID
    
    // 比较函数
    static bool buyOrderComparator(const std::shared_ptr<Order>& a, const std::shared_ptr<Order>& b) {
        if (a->price != b->price) {
            return a->price < b->price;  // 价格优先，高价格买单优先
        }
        return a->timestamp > b->timestamp;  // 时间优先，先到的优先
    }
    
    static bool sellOrderComparator(const std::shared_ptr<Order>& a, const std::shared_ptr<Order>& b) {
        if (a->price != b->price) {
            return a->price > b->price;  // 价格优先，低价格卖单优先
        }
        return a->timestamp > b->timestamp;  // 时间优先，先到的优先
    }

public:
    MatchingEngine() : 
        buyOrders(buyOrderComparator),
        sellOrders(sellOrderComparator),
        nextOrderId(1) {}
    
    // 提交订单
    void submitOrder(OrderType type, double price, int quantity, int timestamp)
     {
        auto order = std::make_shared<Order>(nextOrderId++, type, price, quantity, timestamp);
        
        if (type == OrderType::BUY) 
        {
            processBuyOrder(order);
        } else 
        {
            processSellOrder(order);
        }
    }
    
    // 处理买单
    void processBuyOrder(std::shared_ptr<Order> buyOrder) 
    {
        // 检查是否可以与卖盘中的订单撮合
        while (!sellOrders.empty() && buyOrder->quantity > 0) {
            auto bestSell = sellOrders.top();
            
            // 如果买单价格 >= 卖单价格，可以撮合
            if (buyOrder->price >= bestSell->price) {
                int tradeQty = std::min(buyOrder->quantity, bestSell->quantity);
                double tradePrice = bestSell->price;  // 通常以卖单价格成交
                
                // 记录成交
                tradeHistory.emplace_back(buyOrder->id, bestSell->id, tradePrice, tradeQty, buyOrder->timestamp);
                
                // 更新订单数量
                buyOrder->quantity -= tradeQty;
                bestSell->quantity -= tradeQty;
                
                std::cout << "成交: 买单" << buyOrder->id << " 与 卖单" << bestSell->id 
                         << " 价格:" << tradePrice << " 数量:" << tradeQty << std::endl;
                
                // 如果卖单完全成交，从队列中移除
                if (bestSell->quantity <= 0) {
                    sellOrders.pop();
                } else {
                    // 更新卖单数量（实际应用中需要更新队列，这里简化处理）
                    auto updatedSell = std::make_shared<Order>(
                        bestSell->id, bestSell->type, bestSell->price, bestSell->quantity, bestSell->timestamp);
                    sellOrders.pop();
                    // 重新插入以维护堆结构
                    sellOrders.push(updatedSell);
                }
            } else {
                // 无法撮合，退出循环
                break;
            }
        }
        
        // 如果买单还有剩余，加入买单队列
        if (buyOrder->quantity > 0) {
            buyOrders.push(buyOrder);
        }
    }
    
    // 处理卖单
    void processSellOrder(std::shared_ptr<Order> sellOrder) 
    {
        // 检查是否可以与买盘中的订单撮合
        while (!buyOrders.empty() && sellOrder->quantity > 0)
         {
            auto bestBuy = buyOrders.top();
            
            // 如果卖单价格 <= 买单价格，可以撮合
            if (sellOrder->price <= bestBuy->price) {
                int tradeQty = std::min(sellOrder->quantity, bestBuy->quantity);
                double tradePrice = bestBuy->price;  // 通常以买单价格成交
                
                // 记录成交
                tradeHistory.emplace_back(bestBuy->id, sellOrder->id, tradePrice, tradeQty, sellOrder->timestamp);
                
                // 更新订单数量
                sellOrder->quantity -= tradeQty;
                bestBuy->quantity -= tradeQty;
                
                std::cout << "成交: 卖单" << sellOrder->id << " 与 买单" << bestBuy->id 
                         << " 价格:" << tradePrice << " 数量:" << tradeQty << std::endl;
                
                // 如果买单完全成交，从队列中移除
                if (bestBuy->quantity <= 0) {
                    buyOrders.pop();
                } else {
                    // 更新买单数量
                    auto updatedBuy = std::make_shared<Order>(
                        bestBuy->id, bestBuy->type, bestBuy->price, bestBuy->quantity, bestBuy->timestamp);
                    buyOrders.pop();
                    buyOrders.push(updatedBuy);
                }
            } else {
                // 无法撮合，退出循环
                break;
            }
        }
        
        // 如果卖单还有剩余，加入卖单队列
        if (sellOrder->quantity > 0) 
        {
            sellOrders.push(sellOrder);
        }
    }
    
    // 获取市场深度
    void printMarketDepth()
     {
        std::cout << "\n=== 市场深度 ===" << std::endl;
        
        std::cout << "卖盘:" << std::endl;
        auto sellCopy = sellOrders;
        std::vector<std::shared_ptr<Order>> tempSells;
        while (!sellCopy.empty()) {
            auto order = sellCopy.top();
            std::cout << "价格: " << order->price << ", 数量: " << order->quantity << std::endl;
            tempSells.push_back(order);
            sellCopy.pop();
        }
        
        std::cout << "买盘:" << std::endl;
        auto buyCopy = buyOrders;
        std::vector<std::shared_ptr<Order>> tempBuys;
        while (!buyCopy.empty()) {
            auto order = buyCopy.top();
            std::cout << "价格: " << order->price << ", 数量: " << order->quantity << std::endl;
            tempBuys.push_back(order);
            buyCopy.pop();
        }
    }
    
    // 获取成交历史
    const std::vector<Trade>& getTradeHistory() const 
    {
        return tradeHistory;
    }
};