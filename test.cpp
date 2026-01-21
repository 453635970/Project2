#include <gtest/gtest.h>
#include <string>
#include <fstream>
#include <sstream>
#include "hangqin.hpp"


// 待测试函数：加法
int add(int a, int b) {
    return a + b;
}

// 读取 Windows 文本文件（处理 UTF-8 BOM 和 CRLF -> LF）
// 注意：本函数假定文本为 UTF-8 或 ANSI。若文件为 UTF-16/UTF-32，请先转换编码再读取。
std::string readWindowsTextFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return std::string();
    std::ostringstream ss;
    ss << in.rdbuf();
    std::string s = ss.str();
    // 移除 UTF-8 BOM (0xEF,0xBB,0xBF)
    if (s.size() >= 3 && (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF) {
        s.erase(0, 3);
    }
    // 将 CRLF -> LF: 删除所有 '\r'
    std::string out;
    out.reserve(s.size());
    for (char c : s) if (c != '\r') out.push_back(c);
    return out;
}

// 测试用例 1：正数相加
TEST(AddTest, Positive) {
    EXPECT_EQ(add(1, 2), 3);
    ASSERT_EQ(add(10, 20), 30);
}

// 测试用例 2：负数相加
TEST(AddTest, Negative) {
    EXPECT_EQ(add(-1, -2), -3);
    EXPECT_EQ(add(-5, 3), -2);
}

// // 添加 main 函数以运行所有测试
// int main(int argc, char **argv) 
// {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// } 


int main()
 {
    system("chcp 65001"); // 设置控制台为UTF-8编码
    
    MatchingEngine engine;
    
    // 模拟提交订单
    std::cout << "=== 模拟交易开始 ===" << std::endl;
    
    // 提交卖单
    engine.submitOrder(OrderType::SELL, 100.0, 50, 1);  // 卖单1
    engine.submitOrder(OrderType::SELL, 101.0, 30, 2);  // 卖单2
    engine.submitOrder(OrderType::SELL, 99.0, 20, 3);   // 卖单3
    
    // 提交买单
    engine.submitOrder(OrderType::BUY, 100.0, 40, 4);   // 买单1 - 与卖单3部分成交
    engine.submitOrder(OrderType::BUY, 101.0, 60, 5);   // 买单2 - 与剩余卖单成交
    
    // 显示市场深度
    engine.printMarketDepth();
    
    // 显示成交历史
    std::cout << "\n=== 成交历史 ===" << std::endl;
    for (const auto& trade : engine.getTradeHistory()) {
        std::cout << "买单" << trade.buyOrderId << " 与 卖单" << trade.sellOrderId 
                 << " 成交，价格:" << trade.price << " 数量:" << trade.quantity << std::endl;
    }
    
    return 0;
}