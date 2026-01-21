#include <gtest/gtest.h>
#include "websocket_client.h"
#include <cstdlib>

// 检查默认构造状态
TEST(WebSocketClientTest, DefaultState) {
    WebSocketClient ws;
    EXPECT_FALSE(ws.isConnected());
    EXPECT_EQ(ws.recvText(), "");
    EXPECT_FALSE(ws.sendText("hello"));
}

// close 在未连接时应保持安全
TEST(WebSocketClientTest, CloseBeforeConnect) {
    WebSocketClient ws;
    ws.close();
    EXPECT_FALSE(ws.isConnected());
}

// 连接到不存在的主机应失败
TEST(WebSocketClientTest, ConnectionFailure) {
    WebSocketClient ws;
    // 使用明显不存在的主机名或端口
    EXPECT_TRUE(ws.connect("invalid.nonexistent.host", "12345", "/"));
}

// 在未连接时发送/接收应失败或返回空
TEST(WebSocketClientTest, SendRecvNotConnected) {
    WebSocketClient ws;
    EXPECT_FALSE(ws.sendText("msg"));
    EXPECT_EQ(ws.recvText(), "");
}

// 可选的集成测试：回显服务器
// 要启用，请在运行测试前设置环境变量 WS_ECHO_HOST 和 WS_ECHO_PORT
TEST(WebSocketClientTest, EchoIntegration) {
    const char* host = std::getenv("WS_ECHO_HOST");
    const char* port = std::getenv("WS_ECHO_PORT");
    if (!host || !port) {
        GTEST_SKIP() << "WS_ECHO_HOST/WS_ECHO_PORT not set; skipping integration test";
    }

    WebSocketClient ws;
    ASSERT_TRUE(ws.connect(host, port, "/"));
    ASSERT_TRUE(ws.isConnected());

    std::string msg = "unit-test-echo-\u2603"; // 包含 Unicode 字符用于测试 UTF-8
    ASSERT_TRUE(ws.sendText(msg));

    std::string recv = ws.recvText();
    EXPECT_EQ(recv, msg);

    ws.close();
}
