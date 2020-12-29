#include "GameServer.h"
#include "Client/Server/SpaceLog.h"
#include "Client/Server/WinThread.h"
#include "Client/Server/WinTCPMultiClientServer.h"
#include "Client/Shared/Packets.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

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
    movementUpdateDelta = 1.0f / static_cast<float>(MovementUpdateTickRate);
    if (tcpServer)
    {
        SPACEMMA_WARN("Attempted to start server but it's already up!");
        return false;
    }
    if (MaxClients <= 0 || MaxClients > 255)
    {
        SPACEMMA_ERROR("Invalid MaxClients value ({})!", MaxClients);
        return false;
    }
    SPACEMMA_DEBUG("Starting server...");
    bufferPool = std::make_unique<BufferPool>(1024 * 1024 * 1024);
    tcpServer = std::make_unique<WinTCPMultiClientServer>(*bufferPool, static_cast<unsigned char>(MaxClients));
    if (tcpServer->bindAndListen(StringCast<ANSICHAR>(*ServerIpAddress).Get(), ServerPort))
    {
        acceptThread = new WinThread();
        acceptThread->run(threadAcceptClients, this);
        SetActorTickEnabled(true);
        return true;
    }
    SPACEMMA_ERROR("Game server initialization failed!");
    bufferPool.reset();
    tcpServer.reset();
    return false;
}

bool AGameServer::isServerRunning() const
{
    return tcpServer != nullptr;
}

bool AGameServer::stopServer()
{
    std::lock_guard lock(startStopMutex);
    SetActorTickEnabled(false);
    SPACEMMA_DEBUG("Stopping server...");
    if (tcpServer)
    {
        SPACEMMA_DEBUG("Interrupting threads...");
        acceptThread->interrupt();
        SPACEMMA_DEBUG("Closing tcpServer...");
        tcpServer->close();
        SPACEMMA_DEBUG("Joining threads...");
        acceptThread->join();
        delete acceptThread;
        SPACEMMA_DEBUG("Closing sendThreads...");
        for (const auto& pair : sendThreads)
        {
            pair.second->interrupt();
            pair.second->join();
            delete pair.second;
        }
        SPACEMMA_DEBUG("Closing receiveThreads...");
        for (const auto& pair : receiveThreads)
        {
            pair.second->interrupt();
            pair.second->join();
            delete pair.second;
        }
        sendThreads.clear();
        receiveThreads.clear();
        SPACEMMA_DEBUG("Removing receivedPackets...");
        for (const auto& pair : receivedPackets)
        {
            bufferPool->freeBuffer(pair.second);
        }
        receivedPackets.clear();
        SPACEMMA_DEBUG("Removing perClientSendBuffers...");
        for (const auto& pair : perClientSendBuffers)
        {
            for (ByteBuffer* buff : pair.second->buffers)
            {
                bufferPool->freeBuffer(buff);
            }
            delete pair.second;
        }
        perClientSendBuffers.clear();
        recentlyMoving.clear();
        recentPositions.clear();
        recentRotations.clear();
        SPACEMMA_DEBUG("Resetting tcpServer...");
        tcpServer.reset();
        SPACEMMA_DEBUG("Resetting bufferPool...");
        bufferPool.reset();
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
    switch (EndPlayReason)
    {
        case EEndPlayReason::Type::EndPlayInEditor:
        case EEndPlayReason::Type::RemovedFromWorld:
            stopServer();
            break;
    }
    Super::EndPlay(EndPlayReason);
}

void AGameServer::threadAcceptClients(gsl::not_null<Thread*> thread, void* server)
{
    AGameServer* srv = reinterpret_cast<AGameServer*>(server);
    SPACEMMA_DEBUG("Starting threadAcceptClients...");
    do
    {
        if (srv->players.size() < srv->MaxClients)
        {
            unsigned short port = srv->tcpServer->acceptClient();
            if (port)
            {
                SPACEMMA_INFO("Accepted client #{}!", port);
                std::lock_guard lock(srv->connectionMutex);
                {
                    std::lock_guard lock1(srv->liveClientsMutex);
                    srv->liveClients.insert(port);
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
                }
                srv->sendPacketTo(port, S2C_ProvidePlayerId{ S2C_HProvidePlayerId, {}, port });
                std::lock_guard lock2(srv->spawnAwaitingMutex);
                srv->playersAwaitingSpawn.insert(port);
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
            // Sleep below simulates lag
            //Sleep(100);
            bool sent = srv->tcpServer->sendTo(currentBuff, port);
            //SPACEMMA_DEBUG("Sent packet {} to {}!", *reinterpret_cast<uint8_t*>(currentBuff->getPointer()), port);
            srv->bufferPool->freeBuffer(currentBuff);
            currentBuff = nullptr;
            if (!sent && !srv->tcpServer->isConnected(port))
            {
                SPACEMMA_ERROR("Connection to {} lost.", port);
                if (srv->isClientAvailable(port))
                {
                    std::lock_guard lock(srv->disconnectMutex);
                    srv->disconnectingPlayers.insert(port);
                }
                break;
            }
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
        ByteBuffer* buffer = srv->tcpServer->receiveFrom(port);
        if (buffer)
        {
            std::lock_guard lock(srv->receiveMutex);
            srv->receivedPackets.push_back({ port, buffer });
        } else if (!srv->tcpServer->isConnected(port))
        {
            SPACEMMA_ERROR("Connection to {} lost.", port);
            std::lock_guard lock(srv->disconnectMutex);
            srv->disconnectingPlayers.insert(port);
            break;
        }
    } while (!thread->isInterrupted());
    SPACEMMA_DEBUG("Stopping threadReceive({})...", port);
}

void AGameServer::sendToAll(gsl::not_null<ByteBuffer*> buffer)
{
    std::lock_guard lock(connectionMutex);
    for (const auto& pair : perClientSendBuffers)
    {
        //todo: implement refcounted buffers or something like that, right now the buffer is just copied for each client
        ByteBuffer* buff = bufferPool->getBuffer(buffer->getUsedSize());
        memcpy(buff->getPointer(), buffer->getPointer(), buffer->getUsedSize());
        sendTo(pair.first, buff);
    }
}

void AGameServer::sendToAllBut(gsl::not_null<ByteBuffer*> buffer, unsigned short ignoredClient)
{
    std::lock_guard lock(connectionMutex);
    for (const auto& pair : perClientSendBuffers)
    {
        if (pair.first != ignoredClient)
        {
            //todo: implement refcounted buffers or something like that, right now the buffer is just copied for each client
            ByteBuffer* buff = bufferPool->getBuffer(buffer->getUsedSize());
            memcpy(buff->getPointer(), buffer->getPointer(), buffer->getUsedSize());
            sendTo(pair.first, buff);
        }
    }
}

void AGameServer::sendToAllBut(gsl::not_null<spacemma::ByteBuffer*> buffer, unsigned short ignoredClient1, unsigned short ignoredClient2)
{
    std::lock_guard lock(connectionMutex);
    for (const auto& pair : perClientSendBuffers)
    {
        if (pair.first != ignoredClient1 && pair.first != ignoredClient2)
        {
            //todo: implement refcounted buffers or something like that, right now the buffer is just copied for each client
            ByteBuffer* buff = bufferPool->getBuffer(buffer->getUsedSize());
            memcpy(buff->getPointer(), buffer->getPointer(), buffer->getUsedSize());
            sendTo(pair.first, buff);
        }
    }
}

void AGameServer::sendTo(unsigned short client, gsl::not_null<ByteBuffer*> buffer)
{
    ClientBuffers* buffers = perClientSendBuffers[client];
    std::lock_guard lock(buffers->bufferMutex);
    buffers->buffers.push_back(buffer);
}

void AGameServer::disconnectClient(unsigned short client)
{
    SPACEMMA_WARN("Disconnecting player {}...", client);
    {
        std::lock_guard lock(liveClientsMutex);
        const auto lcit = liveClients.find(client);
        if (lcit != liveClients.end())
        {
            SPACEMMA_DEBUG("Removing liveClients reference...");
            liveClients.erase(lcit);
        }
    }
    {
        const std::map<unsigned short, Thread*>::iterator pair = receiveThreads.find(client);
        if (pair != receiveThreads.end())
        {
            SPACEMMA_DEBUG("Stopping player's receiveThread...");
            pair->second->interrupt();
            pair->second->join();
            delete pair->second;
            receiveThreads.erase(pair);
        }
    }
    {
        const std::map<unsigned short, Thread*>::iterator pair = sendThreads.find(client);
        const std::map<unsigned short, ClientBuffers*>::iterator pair2 = perClientSendBuffers.find(client);
        if (pair != sendThreads.end())
        {
            SPACEMMA_DEBUG("Stopping player's sendThread...");
            pair->second->interrupt();
            pair->second->join();
            delete pair->second;
            sendThreads.erase(pair);
        }
        if (pair2 != perClientSendBuffers.end())
        {
            SPACEMMA_DEBUG("Removing player's send buffers...");
            for (auto buff : pair2->second->buffers)
            {
                bufferPool->freeBuffer(buff);
            }
            perClientSendBuffers.erase(pair2);
        }
    }
    {
        const auto& pair1 = recentlyMoving.find(client);
        if (pair1 != recentlyMoving.end())
        {
            recentlyMoving.erase(pair1);
        }
        const auto& pair2 = recentPositions.find(client);
        if (pair2 != recentPositions.end())
        {
            recentPositions.erase(pair2);
        }
        const auto& pair3 = recentRotations.find(client);
        if (pair3 != recentRotations.end())
        {
            recentRotations.erase(pair3);
        }
    }
    {
        const std::map<unsigned short, AShooterPlayer*>::iterator pair = players.find(client);
        if (pair != players.end())
        {
            SPACEMMA_DEBUG("Removing player's actor...");
            pair->second->Destroy();
            players.erase(pair);
            sendPacketToAll(S2C_DestroyPlayer{ S2C_HDestroyPlayer, {}, client });
        }
    }
}

void AGameServer::processPacket(unsigned short sourceClient, gsl::not_null<ByteBuffer*> buffer)
{
    if (!isClientAvailable(sourceClient))
    {
        SPACEMMA_WARN("Rejecting packet from client {} that is no longer available.", sourceClient);
        return;
    }
    Header header = static_cast<Header>(*buffer->getPointer());
    switch (header)
    {
        case B2B_HShoot:
        {
            B2B_Shoot* packet = reinterpretPacket<B2B_Shoot>(buffer);
            if (packet)
            {
                SPACEMMA_TRACE("B2B_Shoot: {}, [{},{},{}], [{},{},{}]",
                               packet->playerId, packet->location.x, packet->location.y, packet->location.z,
                               packet->rotator.pitch, packet->rotator.yaw, packet->rotator.roll);
            }
            break;
        }
        case B2B_HUpdateVelocity:
        {
            B2B_UpdateVelocity* packet = reinterpretPacket<B2B_UpdateVelocity>(buffer);
            if (packet)
            {
                SPACEMMA_DEBUG("B2B_UpdateVelocity: {}, [{},{},{}]",
                               packet->playerId, packet->velocity.x, packet->velocity.y, packet->velocity.z);
                const std::map<unsigned short, AShooterPlayer*>::iterator pair = players.find(packet->playerId);
                if (pair != players.end())
                {
                    pair->second->GetCharacterMovement()->Velocity = (packet->velocity.asFVector());
                } else
                {
                    SPACEMMA_WARN("Failed to update velocity of {}. Player not found!", packet->playerId);
                }
                sendToAllBut(buffer, sourceClient);
            }
            break;
        }
        case B2B_HRotate:
        {
            B2B_Rotate* packet = reinterpretPacket<B2B_Rotate>(buffer);
            if (packet)
            {
                SPACEMMA_TRACE("B2B_Rotate: {}, [{},{},{}]", packet->playerId,
                               packet->rotator.pitch, packet->rotator.yaw, packet->rotator.roll);
                const std::map<unsigned short, AShooterPlayer*>::iterator pair = players.find(packet->playerId);
                if (pair != players.end())
                {
                    pair->second->SetActorRotation(packet->rotator.asFRotator());
                } else
                {
                    SPACEMMA_WARN("Failed to change rotation of {}. Player not found!", packet->playerId);
                }
                sendToAllBut(buffer, sourceClient);
            }
            break;
        }
        case B2B_HRopeAttach:
        {
            B2B_RopeAttach* packet = reinterpretPacket<B2B_RopeAttach>(buffer);
            if (packet)
            {
                SPACEMMA_TRACE("B2B_RopeAttach: {}, [{},{},{}]", packet->playerId,
                               packet->attachPosition.x, packet->attachPosition.y, packet->attachPosition.z);
                const std::map<unsigned short, AShooterPlayer*>::iterator pair = players.find(packet->playerId);
                if (pair != players.end())
                {
                    pair->second->AttachRope(packet->attachPosition.asFVector(), false);
                } else
                {
                    SPACEMMA_WARN("Failed to attach rope for {}. Player not found!", packet->playerId);
                }
                sendToAllBut(buffer, sourceClient);
            }
            break;
        }
        case B2B_HRopeDetach:
        {
            B2B_RopeDetach* packet = reinterpretPacket<B2B_RopeDetach>(buffer);
            if (packet)
            {
                SPACEMMA_TRACE("B2B_RopeDetach: {}", packet->playerId);
                const std::map<unsigned short, AShooterPlayer*>::iterator pair = players.find(packet->playerId);
                if (pair != players.end())
                {
                    pair->second->DetachRope(false);
                } else
                {
                    SPACEMMA_WARN("Failed to detach rope for {}. Player not found!", packet->playerId);
                }
                sendToAllBut(buffer, sourceClient);
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

void AGameServer::handlePlayerAwaitingSpawn()
{
    unsigned short clientPort = 0;
    {
        std::lock_guard lock(spawnAwaitingMutex);
        if (!playersAwaitingSpawn.empty())
        {
            clientPort = *playersAwaitingSpawn.begin();
            playersAwaitingSpawn.erase(playersAwaitingSpawn.begin());
        }
    }
    if (clientPort)
    {
        SPACEMMA_DEBUG("Spawning player {}...", clientPort);
        FActorSpawnParameters params{};
        params.Name = FName(FString::Printf(TEXT("Player #%d"), static_cast<int32>(clientPort)));
        params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        AShooterPlayer* actor = GetWorld()->SpawnActor<AShooterPlayer>(PlayerBP, FVector{ 100.0f, 100.0f, 100.0f },
                                                                       FRotator{}, params);

        sendPacketToAll(S2C_CreatePlayer{ S2C_HCreatePlayer, {}, clientPort,
                        actor->GetActorLocation(), actor->GetActorRotation() });
        // tell the player about other players
        for (const auto& pair : players)
        {
            sendPacketTo(clientPort, S2C_CreatePlayer{ S2C_HCreatePlayer, {}, pair.first,
                             pair.second->GetActorLocation(), pair.second->GetActorRotation() });
        }
        players.emplace(clientPort, actor);
        recentPositions.emplace(clientPort, actor->GetActorLocation());
        recentRotations.emplace(clientPort, actor->GetActorRotation());
        recentlyMoving.emplace(clientPort, false);
    }
}

void AGameServer::handlePendingDisconnect()
{
    unsigned short clientPortToDisconnect = 0;
    {
        std::lock_guard lock(disconnectMutex);
        if (!disconnectingPlayers.empty())
        {
            clientPortToDisconnect = *disconnectingPlayers.begin();
            disconnectingPlayers.erase(disconnectingPlayers.begin());
        }
    }
    if (clientPortToDisconnect)
    {
        disconnectClient(clientPortToDisconnect);
    }
}

void AGameServer::processPendingPacket()
{
    ByteBuffer* buff = nullptr;
    unsigned short clientPort = 0;
    {
        std::lock_guard lock(receiveMutex);
        if (!receivedPackets.empty())
        {
            clientPort = receivedPackets[0].first;
            buff = receivedPackets[0].second;
            receivedPackets.erase(receivedPackets.begin());
        }
    }
    if (buff)
    {
        processPacket(clientPort, buff);
        bufferPool->freeBuffer(buff);
    }
}

void AGameServer::processAllPendingPackets()
{
    while (!receivedPackets.empty())
    {
        processPendingPacket();
    }
}

void AGameServer::broadcastPlayerMovement(unsigned short client)
{
    const auto& pair = players.find(client);
    if (pair != players.end())
    {
        sendPacketToAll(S2C_PlayerMovement{ S2C_HPlayerMovement, {}, client, pair->second->GetActorLocation(),
                           pair->second->GetActorRotation(), pair->second->GetCharacterMovement()->Velocity });
    } else
    {
        SPACEMMA_WARN("Failed to broadcast movement of {}. Player not found.", client);
    }
}

void AGameServer::broadcastMovingPlayers()
{
    for (std::map<unsigned short, bool>::value_type& pair : recentlyMoving)
    {
        const std::map<unsigned short, AShooterPlayer*>::iterator& playerPair = players.find(pair.first);
        if (playerPair != players.end())
        {
            FVector location = playerPair->second->GetActorLocation();
            FRotator rotation = playerPair->second->GetActorRotation();
            bool recentlyMoved = pair.second;
            bool currentlyMoved =
                !playerPair->second->GetCharacterMovement()->Velocity.IsNearlyZero() ||
                //!playerPair->second->GetRotationVector().IsNearlyZero() ||
                location != recentPositions[pair.first] ||
                rotation != recentRotations[pair.first];
            if (recentlyMoved || currentlyMoved)
            {
                recentPositions[pair.first] = location;
                recentRotations[pair.first] = rotation;
                broadcastPlayerMovement(pair.first);
                pair.second = currentlyMoved;
            }
        } else
        {
            SPACEMMA_WARN("Player {} not found!", pair.first);
        }
    }
}

bool AGameServer::isClientAvailable(unsigned short client)
{
    std::lock_guard lock(liveClientsMutex);
    return liveClients.find(client) != liveClients.end();
}

void AGameServer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (isServerRunning())
    {
        handlePlayerAwaitingSpawn();
        processAllPendingPackets();
        handlePendingDisconnect();
        currentMovementUpdateDelta += DeltaTime;
        if (currentMovementUpdateDelta >= movementUpdateDelta)
        {
            currentMovementUpdateDelta -= movementUpdateDelta;
            broadcastMovingPlayers();
        }
    }
}

