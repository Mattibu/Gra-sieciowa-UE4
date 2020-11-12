#include "pch.h"
#include "WinTCPClient.h"
#include "WinTCPServer.h"
#include "WinTCPMultiClientServer.h"

#include <thread>

using namespace spacemma;

const char* testAddress{ "127.0.0.1" };
const unsigned short testPort{ 9876 };
const unsigned char MAX_CLIENTS{ 2U };
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

TEST(TCPClientServerTest, MultiClientServerTest)
{
    BufferPool bufferPool(1024);
    WinTCPMultiClientServer server(bufferPool, MAX_CLIENTS);
    WinTCPClient client1(bufferPool), client2(bufferPool);
    ASSERT_FALSE(server.isListening());
    ASSERT_FALSE(server.isConnected());
    ASSERT_TRUE(server.bindAndListen(testAddress, testPort));
    ASSERT_TRUE(server.isListening());
    ASSERT_FALSE(server.isConnected());
    ASSERT_EQ(0U, server.getClientCount());
    bool accepted = false;
    std::thread acceptThread([&]() { accepted = server.acceptClient(); });
    ASSERT_FALSE(accepted);
    bool connected = false;
    ASSERT_FALSE(client1.isConnected());
    std::thread connThread([&]() { connected = client1.connect(testAddress, testPort); });
    Sleep(10);
    ASSERT_TRUE(accepted);
    ASSERT_TRUE(connected);
    ASSERT_TRUE(server.isConnected());
    ASSERT_EQ(1U, server.getClientCount());
    ASSERT_TRUE(server.isListening());
    ASSERT_TRUE(client1.isConnected());
    ASSERT_TRUE(acceptThread.joinable());
    ASSERT_TRUE(connThread.joinable());
    acceptThread.join();
    connThread.join();
    std::vector<unsigned short> clientPorts = server.getClientPorts();
    ASSERT_EQ(1, clientPorts.size());
    ASSERT_NE(0U, clientPorts[0]);
    unsigned short port1 = clientPorts[0];
    accepted = false;
    connected = false;
    acceptThread = std::thread([&]() { accepted = server.acceptClient(); });
    ASSERT_FALSE(accepted);
    ASSERT_FALSE(client2.isConnected());
    connThread = std::thread([&]() { connected = client2.connect(testAddress, testPort); });
    Sleep(10);
    ASSERT_TRUE(accepted);
    ASSERT_TRUE(connected);
    ASSERT_TRUE(server.isConnected());
    ASSERT_EQ(2U, server.getClientCount());
    clientPorts = server.getClientPorts();
    ASSERT_EQ(2, clientPorts.size());
    ASSERT_EQ(port1, clientPorts[0]);
    ASSERT_NE(port1, clientPorts[1]);
    ASSERT_NE(0U, clientPorts[1]);
    unsigned short port2 = clientPorts[1];
    ASSERT_TRUE(server.isListening());
    ASSERT_TRUE(client2.isConnected());
    ASSERT_TRUE(acceptThread.joinable());
    ASSERT_TRUE(connThread.joinable());
    acceptThread.join();
    connThread.join();
    ByteBuffer* buff{ bufferPool.getBuffer(TEST_BUFF_SIZE) };
    memcpy(buff->getPointer(), testBuff, TEST_BUFF_SIZE);
    ASSERT_TRUE(server.send(buff));
    bufferPool.freeBuffer(buff);
    buff = client1.receive();
    ASSERT_NE(nullptr, buff);
    ASSERT_EQ(buff->getUsedSize(), TEST_BUFF_SIZE);
    ASSERT_EQ(0, memcmp(testBuff, buff->getPointer(), TEST_BUFF_SIZE));
    bufferPool.freeBuffer(buff);
    buff = client2.receive();
    ASSERT_NE(nullptr, buff);
    ASSERT_EQ(buff->getUsedSize(), TEST_BUFF_SIZE);
    ASSERT_EQ(0, memcmp(testBuff, buff->getPointer(), TEST_BUFF_SIZE));
    ASSERT_EQ(nullptr, server.receive());
    ASSERT_TRUE(client1.send(buff));
    ASSERT_TRUE(client2.send(buff));
    bufferPool.freeBuffer(buff);
    buff = server.receive(port1);
    ASSERT_NE(nullptr, buff);
    ASSERT_EQ(buff->getUsedSize(), TEST_BUFF_SIZE);
    ASSERT_EQ(0, memcmp(testBuff, buff->getPointer(), TEST_BUFF_SIZE));
    bufferPool.freeBuffer(buff);
    buff = server.receive(port2);
    ASSERT_NE(nullptr, buff);
    ASSERT_EQ(buff->getUsedSize(), TEST_BUFF_SIZE);
    ASSERT_EQ(0, memcmp(testBuff, buff->getPointer(), TEST_BUFF_SIZE));
    ASSERT_TRUE(server.send(buff, port1));
    bufferPool.freeBuffer(buff);
    buff = client1.receive();
    ASSERT_NE(nullptr, buff);
    ASSERT_EQ(buff->getUsedSize(), TEST_BUFF_SIZE);
    ASSERT_EQ(0, memcmp(testBuff, buff->getPointer(), TEST_BUFF_SIZE));
    ASSERT_TRUE(server.send(buff, port2));
    bufferPool.freeBuffer(buff);
    buff = client2.receive();
    ASSERT_NE(nullptr, buff);
    ASSERT_EQ(buff->getUsedSize(), TEST_BUFF_SIZE);
    ASSERT_EQ(0, memcmp(testBuff, buff->getPointer(), TEST_BUFF_SIZE));
    bufferPool.freeBuffer(buff);
    ASSERT_TRUE(client1.shutdown());
    ASSERT_TRUE(client1.close());
    ASSERT_TRUE(client2.shutdown());
    ASSERT_TRUE(client2.close());
    //ASSERT_TRUE(server.shutdown());
    server.shutdown();
    ASSERT_TRUE(server.close());
}