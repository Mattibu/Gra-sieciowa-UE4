#include "GameServer.h"
#include "Client/Server/SpaceLog.h"
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

bool AGameServer::startServer()
{
    std::lock_guard lock(startStopMutex);
    if (serverActive)
    {
        SPACEMMA_WARN("Attempted to start server but it's already running!");
        return false;
    }
    SPACEMMA_DEBUG("Starting server...");
    if (tcpServer.bindAndListen(StringCast<ANSICHAR>(*ServerIpAddress).Get(), ServerPort))
    {
        acceptThread = new WinThread();
        processPacketsThread = new WinThread();
        serverActive = true;
        acceptThread->run(threadAcceptClients, this);
        processPacketsThread->run(threadProcessPackets, this);
        return true;
    }
    SPACEMMA_ERROR("Game server initialization failed!");
    return false;
}

bool AGameServer::isServerRunning() const
{
    return serverActive;
}

bool AGameServer::stopServer()
{
    std::lock_guard lock(startStopMutex);
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
        return true;
    }
    return false;
}

void AGameServer::BeginPlay()
{
    Super::BeginPlay();
}

void AGameServer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    stopServer();
}

void AGameServer::threadAcceptClients(gsl::not_null<Thread*> thread, void* server)
{
    AGameServer* srv = reinterpret_cast<AGameServer*>(server);
    SPACEMMA_DEBUG("Starting threadAcceptClients...");
    do
    {
        if (srv->players.size() < srv->MAX_CLIENTS)
        {
            unsigned short port = srv->tcpServer.acceptClient();
            if (port)
            {
                SPACEMMA_INFO("Accepted client #{}!", port);
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
                // todo: change local player (same process) detection to something nicer.
                if (srv->players.empty() && srv->LocalPlayer != nullptr)
                {
                    srv->players.emplace(port, srv->LocalPlayer);
                } else
                {
                    // todo: spawn and map APawn! also, notify all players appropriately
                }
                srv->sendPacketTo(port, S2C_ProvidePlayerId{ S2C_HProvidePlayerId, {}, port });
            }
        }
    } while (!thread->isInterrupted());
    SPACEMMA_DEBUG("Stopping threadAcceptClients...");
}

void AGameServer::threadSend(gsl::not_null<Thread*> thread, void* args)
{
    ThreadArgs* arg = reinterpret_cast<ThreadArgs*>(args);
    AGameServer* srv = reinterpret_cast<AGameServer*>(arg->server);
    unsigned short port = arg->port;
    arg->received = true;
    ClientBuffers* cb = srv->perClientSendBuffers[port];
    ByteBuffer* currentBuff{ nullptr };
    SPACEMMA_DEBUG("Starting threadSend({})...", port);
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
                SPACEMMA_ERROR("Failed to send {} data to client {}!", currentBuff->getUsedSize(), port);
            }
            srv->bufferPool.freeBuffer(currentBuff);
            currentBuff = nullptr;
        }
    } while (!thread->isInterrupted());
    SPACEMMA_DEBUG("Stopping threadSend({})...", port);
}

void AGameServer::threadReceive(gsl::not_null<Thread*> thread, void* args)
{
    ThreadArgs* arg = reinterpret_cast<ThreadArgs*>(args);
    AGameServer* srv = reinterpret_cast<AGameServer*>(arg->server);
    unsigned short port = arg->port;
    arg->received = true;
    SPACEMMA_DEBUG("Starting threadReceive({})...", port);
    do
    {
        ByteBuffer* buffer = srv->tcpServer.receive(port);
        if (buffer)
        {
            std::lock_guard lock(srv->receiveMutex);
            srv->receivedPackets.push_back({ port, buffer });
        }
    } while (!thread->isInterrupted());
    SPACEMMA_DEBUG("Stopping threadReceive({})...", port);
}

void AGameServer::threadProcessPackets(gsl::not_null<spacemma::Thread*> thread, void* server)
{
    AGameServer* srv = reinterpret_cast<AGameServer*>(server);
    ByteBuffer* currentBuff{ nullptr };
    unsigned short clientPort{};
    SPACEMMA_DEBUG("Starting threadProcessPackets...");
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
    SPACEMMA_DEBUG("Stopped threadProcessPackets...");
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
                SPACEMMA_DEBUG("B2B_Shoot: {}, [{},{},{}], [{},{},{}]",
                               packet->playerId, packet->location.x, packet->location.y, packet->location.z,
                               packet->rotator.pitch, packet->rotator.yaw, packet->rotator.roll);
            }
            break;
        }
        case B2B_HChangeSpeed:
        {
            B2B_ChangeSpeed* packet = reinterpretPacket<B2B_ChangeSpeed>(buffer);
            if (packet)
            {
                SPACEMMA_DEBUG("B2B_ChangeSpeed: {}, [{},{},{}]",
                               packet->playerId, packet->speedVector.x, packet->speedVector.y, packet->speedVector.z);
            }
            break;
        }
        case B2B_HRotate:
        {
            B2B_Rotate* packet = reinterpretPacket<B2B_Rotate>(buffer);
            if (packet)
            {
                SPACEMMA_DEBUG("B2B_Rotate: {}, [{},{},{}]", packet->playerId,
                               packet->rotationVector.x, packet->rotationVector.y, packet->rotationVector.z);
            }
            break;
        }
        case B2B_HRopeAttach:
        {
            B2B_RopeAttach* packet = reinterpretPacket<B2B_RopeAttach>(buffer);
            if (packet)
            {
                SPACEMMA_DEBUG("B2B_RopeAttach: {}, [{},{},{}]", packet->playerId,
                               packet->attachPosition.x, packet->attachPosition.y, packet->attachPosition.z);
            }
            break;
        }
        case B2B_HRopeDetach:
        {
            B2B_RopeDetach* packet = reinterpretPacket<B2B_RopeDetach>(buffer);
            if (packet)
            {
                SPACEMMA_DEBUG("B2B_RopeDetach: {}", packet->playerId);
            }
            break;
        }
        default:
        {
            SPACEMMA_ERROR("Received invalid packet header of {}! Discarding packet.", header);
            break;
        }
    }
}

void AGameServer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

}

