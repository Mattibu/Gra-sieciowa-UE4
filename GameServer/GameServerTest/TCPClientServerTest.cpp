#include "pch.h"
#include "WinTCPClient.h"
#include "WinTCPServer.h"

#include <thread>

using namespace spacemma;

const char* testAddress{ "127.0.0.1" };
const unsigned short testPort{ 9876 };
constexpr size_t TEST_BUFF_SIZE{ 10ULL };
const uint8_t testBuff[TEST_BUFF_SIZE]{ 0, 10, 0, 40, 250, 60, 20, 120, 180, 128 };

TEST(TCPClientServerTest, BasicClientServerTest)
{
    BufferPool bufferPool(1024);
    WinTCPServer server(bufferPool);
    WinTCPClient client(bufferPool);
    ASSERT_FALSE(server.isListening());
    ASSERT_FALSE(server.isConnected());
    ASSERT_TRUE(server.bindAndListen(testAddress, testPort));
    ASSERT_TRUE(server.isListening());
    ASSERT_FALSE(server.isConnected());
    bool accepted = false;
    std::thread acceptThread([&]() { accepted = server.acceptClient(); });
    ASSERT_FALSE(accepted);
    bool connected = false;
    ASSERT_FALSE(client.isConnected());
    std::thread connThread([&]() { connected = client.connect(testAddress, testPort); });
    Sleep(10);
    ASSERT_TRUE(accepted);
    ASSERT_TRUE(connected);
    ASSERT_TRUE(server.isConnected());
    ASSERT_FALSE(server.isListening());
    ASSERT_TRUE(client.isConnected());
    ASSERT_TRUE(acceptThread.joinable());
    ASSERT_TRUE(connThread.joinable());
    acceptThread.join();
    connThread.join();
    ByteBuffer* buff{ bufferPool.getBuffer(TEST_BUFF_SIZE) };
    memcpy(buff->getPointer(), testBuff, TEST_BUFF_SIZE);
    ASSERT_TRUE(server.send(buff));
    bufferPool.freeBuffer(buff);
    buff = client.receive();
    ASSERT_NE(nullptr, buff);
    ASSERT_EQ(buff->getUsedSize(), TEST_BUFF_SIZE);
    ASSERT_EQ(0, memcmp(testBuff, buff->getPointer(), TEST_BUFF_SIZE));
    bufferPool.freeBuffer(buff);
    ASSERT_TRUE(client.shutdown());
    ASSERT_TRUE(client.close());
    ASSERT_TRUE(server.shutdown());
    ASSERT_TRUE(server.close());
}