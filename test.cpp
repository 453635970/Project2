#include <gtest/gtest.h>
#include <string>
#include <fstream>
#include <sstream>

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

// 添加 main 函数以运行所有测试
int main(int argc, char **argv) 
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 


