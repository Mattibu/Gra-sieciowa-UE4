#include "GameServer.h"
#include "Client/Server/ServerLog.h"
#include "Client/Server/WinThread.h"
#include "Client/Shared/Packets.h"

using namespace spacemma;

struct ThreadArgs
{
    AGameServer* server{};
    unsigned short port{};
    std::atomic_bool received{ false };
};

AGameServer::AGameServer()
{
    PrimaryActorTick.bCanEverTick = true;

}

void AGameServer::BeginPlay()
{
    Super::BeginPlay();
    if (tcpServer.bindAndListen("127.0.0.1", 4444))
    {
        acceptThread = new WinThread();
        processPacketsThread = new WinThread();
        serverActive = true;
        acceptThread->run(threadAcceptClients, this);
        processPacketsThread->run(threadProcessPackets, this);
    } else
    {
        SERVER_ERROR("Game server initialization failed!");
    }
}

void AGameServer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    if (serverActive)
    {
        serverActive = false;
        acceptThread->interrupt();
        processPacketsThread->interrupt();
        tcpServer.close();
        acceptThread->join();
        processPacketsThread->join();
        delete acceptThread;
        delete processPacketsThread;
        for (const auto& pair : sendThreads)
        {
            pair.second->interrupt();
            pair.second->join();
            delete pair.second;
        }
        for (const auto& pair : receiveThreads)
        {
            pair.second->interrupt();
            pair.second->join();
            delete pair.second;
        }
        sendThreads.clear();
        receiveThreads.clear();
        for (const auto& pair : receivedPackets)
        {
            bufferPool.freeBuffer(pair.second);
        }
        receivedPackets.clear();
        for (const auto& pair : perClientSendBuffers)
        {
            for (ByteBuffer* buff : pair.second->buffers)
            {
                bufferPool.freeBuffer(buff);
            }
            delete pair.second;
        }
        perClientSendBuffers.clear();
    }
}

void AGameServer::threadAcceptClients(gsl::not_null<Thread*> thread, void* server)
{
    AGameServer* srv = reinterpret_cast<AGameServer*>(server);
    do
    {
        if (srv->players.size() < srv->MAX_CLIENTS)
        {
            unsigned short port = srv->tcpServer.acceptClient();
            if (port)
            {
                std::lock_guard lock(srv->connectionMutex);
                srv->perClientSendBuffers.insert({ port, new ClientBuffers{} });
                Thread
                    * sendThread = new WinThread(),
                    * receiveThread = new WinThread();
                ThreadArgs args{ srv, port, false };
                sendThread->run(threadSend, &args);
                while (!args.received);
                args.received = false;
                receiveThread->run(threadReceive, &args);
                while (!args.received);
                srv->sendThreads.emplace(port, sendThread);
                srv->receiveThreads.emplace(port, receiveThread);
                // todo: create and map AActor!
            }
        }
    } while (!thread->isInterrupted());
}

void AGameServer::threadSend(gsl::not_null<Thread*> thread, void* args)
{
    ThreadArgs* arg = reinterpret_cast<ThreadArgs*>(args);
    AGameServer* srv = reinterpret_cast<AGameServer*>(arg->server);
    unsigned short port = arg->port;
    arg->received = true;
    ClientBuffers* cb = srv->perClientSendBuffers[port];
    ByteBuffer* currentBuff{ nullptr };
    do
    {
        {
            std::lock_guard lock(cb->bufferMutex);
            if (!cb->buffers.empty())
            {
                currentBuff = cb->buffers[0];
                cb->buffers.erase(cb->buffers.begin());
            }
        }
        if (currentBuff)
        {
            if (!srv->tcpServer.send(currentBuff, port))
            {
                SERVER_ERROR("Failed to send {} data to client {}!", currentBuff->getUsedSize(), port);
            }
            srv->bufferPool.freeBuffer(currentBuff);
            currentBuff = nullptr;
        }
    } while (!thread->isInterrupted());
}

void AGameServer::threadReceive(gsl::not_null<Thread*> thread, void* args)
{
    ThreadArgs* arg = reinterpret_cast<ThreadArgs*>(args);
    AGameServer* srv = reinterpret_cast<AGameServer*>(arg->server);
    unsigned short port = arg->port;
    arg->received = true;
    do
    {
        ByteBuffer* buffer = srv->tcpServer.receive(port);
        if (buffer)
        {
            std::lock_guard lock(srv->receiveMutex);
            srv->receivedPackets.push_back({ port, buffer });
        }
    } while (!thread->isInterrupted());
}

void AGameServer::threadProcessPackets(gsl::not_null<spacemma::Thread*> thread, void* server)
{
    AGameServer* srv = reinterpret_cast<AGameServer*>(server);
    ByteBuffer* currentBuff{ nullptr };
    unsigned short clientPort{};
    do
    {
        {
            std::lock_guard lock(srv->receiveMutex);
            if (!srv->receivedPackets.empty())
            {
                clientPort = srv->receivedPackets[0].first;
                currentBuff = srv->receivedPackets[0].second;
                srv->receivedPackets.erase(srv->receivedPackets.begin());
            }
        }
        if (currentBuff)
        {
            srv->processPacket(clientPort, currentBuff);
            srv->bufferPool.freeBuffer(currentBuff);
            currentBuff = nullptr;
        }
    } while (!thread->isInterrupted());
}

void AGameServer::sendToAll(gsl::not_null<ByteBuffer*> buffer)
{
    std::lock_guard lock(connectionMutex);
    for (const auto& pair : perClientSendBuffers)
    {
        //todo: implement refcounted buffers or something like that, right now the buffer is just copied for each client
        ByteBuffer* buff = bufferPool.getBuffer(buffer->getUsedSize());
        memcpy(buff->getPointer(), buffer->getPointer(), buffer->getUsedSize());
        sendTo(pair.first, buff);
    }
    bufferPool.freeBuffer(buffer);
}

void AGameServer::sendTo(unsigned short client, gsl::not_null<ByteBuffer*> buffer)
{
    ClientBuffers* buffers = perClientSendBuffers[client];
    std::lock_guard lock(buffers->bufferMutex);
    buffers->buffers.push_back(buffer);
}

void AGameServer::processPacket(unsigned short sourceClient, gsl::not_null<ByteBuffer*> buffer)
{
    Header header = static_cast<Header>(*buffer->getPointer());
    switch (header)
    {
        case B2B_HShoot:
        {
            B2B_Shoot* packet = reinterpretPacket<B2B_Shoot>(buffer);
            if (packet)
            {

            }
            break;
        }
        case B2B_HChangeSpeed:
        {
            B2B_ChangeSpeed* packet = reinterpretPacket<B2B_ChangeSpeed>(buffer);
            if (packet)
            {

            }
            break;
        }
        case B2B_HRotate:
        {
            B2B_Rotate* packet = reinterpretPacket<B2B_Rotate>(buffer);
            if (packet)
            {

            }
            break;
        }
        case B2B_HRopeAttach:
        {
            B2B_RopeAttach* packet = reinterpretPacket<B2B_RopeAttach>(buffer);
            if (packet)
            {

            }
            break;
        }
        case B2B_HRopeDetach:
        {
            B2B_RopeDetach* packet = reinterpretPacket<B2B_RopeDetach>(buffer);
            if (packet)
            {

            }
            break;
        }
        default:
        {
            SERVER_ERROR("Received invalid packet header of {}! Discarding packet.", header);
            break;
        }
    }
}

void AGameServer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

}

