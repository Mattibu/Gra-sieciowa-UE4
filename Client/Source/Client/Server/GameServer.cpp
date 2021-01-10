#include "GameServer.h"
#include "Client/Server/SpaceLog.h"
#include "Client/Server/WinThread.h"
#include "Client/Server/WinTCPMultiClientServer.h"
#include "Client/Shared/Packets.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

using namespace spacemma;
using namespace spacemma::packets;

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
    mapName = StringCast<ANSICHAR>(*GetWorld()->GetName()).Get();
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
        for (const auto& pair : gameClientData)
        {
            SPACEMMA_DEBUG("Cleaning up player {}...");
            SPACEMMA_DEBUG("Closing sendThread...");
            pair.second.sendThread->interrupt();
            pair.second.sendThread->join();
            delete pair.second.sendThread;
            SPACEMMA_DEBUG("Closing receiveThread...");
            pair.second.receiveThread->interrupt();
            pair.second.receiveThread->join();
            delete pair.second.receiveThread;
            SPACEMMA_DEBUG("Removing send buffers...");
            for (ByteBuffer* buff : pair.second.sendBuffers->buffers)
            {
                bufferPool->freeBuffer(buff);
            }
            delete pair.second.sendBuffers;
        }
        SPACEMMA_DEBUG("Removing receivedPackets...");
        for (const auto& pair : receivedPackets)
        {
            bufferPool->freeBuffer(pair.second);
        }
        receivedPackets.clear();
        liveClients.clear();
        playersAwaitingSpawn.clear();
        disconnectingPlayers.clear();
        gameClientData.clear();
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
        if (srv->gameClientData.size() < srv->MaxClients)
        {
            unsigned short port = srv->tcpServer->acceptClient();
            if (port)
            {
                SPACEMMA_INFO("Accepted client #{}!", port);
                std::lock_guard lock(srv->connectionMutex);
                GameClientData data{};
                data.sendThread = new WinThread();
                data.receiveThread = new WinThread();
                data.sendBuffers = new ClientBuffers{};
                srv->gameClientData.insert({ port, data });
                ThreadArgs args{ srv, port, false };
                data.sendThread->run(threadSend, &args);
                while (!args.received);
                args.received = false;
                data.receiveThread->run(threadReceive, &args);
                while (!args.received);
                SPACEMMA_DEBUG("Providing player ID...");
                srv->sendPacketTo(port, S2C_ProvidePlayerId{ S2C_HProvidePlayerId, {}, port });
                std::lock_guard lock1(srv->unverifiedPlayersMutex);
                srv->unverifiedPlayers.insert(port);
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
    ClientBuffers* cb = srv->gameClientData[port].sendBuffers;
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
                if (srv->isClientLive(port))
                {
                    std::lock_guard lock1(srv->disconnectMutex);
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
        }
        else if (!srv->tcpServer->isConnected(port))
        {
            SPACEMMA_ERROR("Connection to {} lost.", port);
            if (srv->isClientLive(port))
            {
                std::lock_guard lock1(srv->disconnectMutex);
                srv->disconnectingPlayers.insert(port);
            }
            break;
        }
    } while (!thread->isInterrupted());
    SPACEMMA_DEBUG("Stopping threadReceive({})...", port);
}

void AGameServer::sendToAll(gsl::not_null<ByteBuffer*> buffer)
{
    std::lock_guard lock(connectionMutex);
    for (unsigned short port : liveClients)
    {
        //todo: implement refcounted buffers or something like that, right now the buffer is just copied for each client
        ByteBuffer* buff = bufferPool->getBuffer(buffer->getUsedSize());
        memcpy(buff->getPointer(), buffer->getPointer(), buffer->getUsedSize());
        sendTo(port, buff);
    }
}

void AGameServer::sendToAllBut(gsl::not_null<ByteBuffer*> buffer, unsigned short ignoredClient)
{
    std::lock_guard lock(connectionMutex);
    for (unsigned short port : liveClients)
    {
        if (port != ignoredClient)
        {
            //todo: implement refcounted buffers or something like that, right now the buffer is just copied for each client
            ByteBuffer* buff = bufferPool->getBuffer(buffer->getUsedSize());
            memcpy(buff->getPointer(), buffer->getPointer(), buffer->getUsedSize());
            sendTo(port, buff);
        }
    }
}

void AGameServer::sendToAllBut(gsl::not_null<ByteBuffer*> buffer, unsigned short ignoredClient1, unsigned short ignoredClient2)
{
    std::lock_guard lock(connectionMutex);
    for (unsigned short port : liveClients)
    {
        if (port != ignoredClient1 && port != ignoredClient2)
        {
            //todo: implement refcounted buffers or something like that, right now the buffer is just copied for each client
            ByteBuffer* buff = bufferPool->getBuffer(buffer->getUsedSize());
            memcpy(buff->getPointer(), buffer->getPointer(), buffer->getUsedSize());
            sendTo(port, buff);
        }
    }
}

void AGameServer::sendTo(unsigned short client, gsl::not_null<ByteBuffer*> buffer)
{
    ClientBuffers* buffers = gameClientData[client].sendBuffers;
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
    const std::map<unsigned short, GameClientData>::iterator& pair = gameClientData.find(client);
    if (pair != gameClientData.end())
    {
        SPACEMMA_DEBUG("Stopping player's receiveThread...");
        tcpServer->closeClient(client);
        pair->second.receiveThread->interrupt();
        pair->second.receiveThread->join();
        delete pair->second.receiveThread;
        SPACEMMA_DEBUG("Stopping player's sendThread...");
        pair->second.sendThread->interrupt();
        pair->second.sendThread->join();
        delete pair->second.sendThread;
        SPACEMMA_DEBUG("Removing player's send buffers...");
        for (auto buff : pair->second.sendBuffers->buffers)
        {
            bufferPool->freeBuffer(buff);
        }
        delete pair->second.sendBuffers;
        if (pair->second.player)
        {
            SPACEMMA_DEBUG("Removing player's actor...");
            pair->second.player->Destroy();
        }
        sendPacketToAll(S2C_DestroyPlayer{ S2C_HDestroyPlayer, {}, client });
    }
    gameClientData.erase(pair);
}

void AGameServer::processPacket(unsigned short sourceClient, gsl::not_null<ByteBuffer*> buffer)
{
    gsl::span<uint8_t> span = buffer->getSpan();
    size_t buffPos = 0;
    bool dataValid = true;
    if (!isClientLive(sourceClient))
    {
        bool unverified;
        {
            std::lock_guard lock(unverifiedPlayersMutex);
            unverified = unverifiedPlayers.find(sourceClient) != unverifiedPlayers.end();
        }
        if (unverified)
        {
            SPACEMMA_INFO("Player {} is unverified. Expecting C2S_InitConnection.");
            Header header = static_cast<Header>(span[buffPos]);
            if (header == C2S_HInitConnection)
            {
                C2S_InitConnection packet;
                if (reinterpretPacket(span, buffPos, packet))
                {
                    SPACEMMA_INFO("C2S_InitConnection: '{}', '{}'", packet.mapName, packet.nickname);
                    bool mapValid = packet.mapName == mapName, nicknameValid = true;
                    if (mapValid)
                    {

                        for (const auto& playerData : gameClientData)
                        {
                            if (playerData.second.nickname == packet.nickname)
                            {
                                nicknameValid = false;
                                break;
                            }
                        }
                    }
                    if (mapValid)
                    {
                        if (nicknameValid)
                        {
                            SPACEMMA_INFO("Player {} identified with nickname '{}'!", sourceClient, packet.nickname);
                            gameClientData[sourceClient].nickname = packet.nickname;
                            std::lock_guard lock1(spawnAwaitingMutex);
                            playersAwaitingSpawn.insert(sourceClient);
                            std::lock_guard lock2(unverifiedPlayersMutex);
                            unverifiedPlayers.erase(sourceClient);
                        }
                        else
                        {
                            SPACEMMA_WARN("Player {} provided invalid nickname '{}'. Closing connection.");
                            ByteBuffer* buff = bufferPool->getBuffer(1);
                            *buff->getPointer() = S2C_HInvalidNickname;
                            tcpServer->sendTo(buff, sourceClient);
                            bufferPool->freeBuffer(buff);
                            std::lock_guard lock(disconnectMutex);
                            disconnectingPlayers.insert(sourceClient);
                            return;
                        }
                    }
                    else
                    {
                        SPACEMMA_WARN("Player {} is playing on a different map ('{}' != '{}'). Closing connection.", sourceClient, mapName, packet.mapName);
                        ByteBuffer* buff = bufferPool->getBuffer(1);
                        *buff->getPointer() = S2C_HInvalidMap;
                        tcpServer->sendTo(buff, sourceClient);
                        bufferPool->freeBuffer(buff);
                        std::lock_guard lock(disconnectMutex);
                        disconnectingPlayers.insert(sourceClient);
                        return;
                    }
                }
                else
                {
                    return;
                }
            }
            else
            {
                SPACEMMA_WARN("Player {} did not authorise. Closing connection.", sourceClient);
                ByteBuffer* buff = bufferPool->getBuffer(1);
                *buff->getPointer() = S2C_HInvalidData;
                tcpServer->sendTo(buff, sourceClient);
                bufferPool->freeBuffer(buff);
                std::lock_guard lock(disconnectMutex);
                disconnectingPlayers.insert(sourceClient);
                return;
            }
        }
        else
        {
            SPACEMMA_WARN("Rejecting packet from client {} that is not live.", sourceClient);
            return;
        }
    }
    while (dataValid && buffPos < span.size())
    {
        Header header = static_cast<Header>(span[buffPos]);
        switch (header)
        {
        case C2S_HShoot:
        {
            C2S_Shoot* packet = reinterpretPacket<C2S_Shoot>(span, buffPos);
            if (packet)
            {
                SPACEMMA_DEBUG("C2S_Shoot: {}, [{},{},{}], [{},{},{}]",
                    packet->playerId, packet->location.x, packet->location.y, packet->location.z,
                    packet->rotator.pitch, packet->rotator.yaw, packet->rotator.roll);
                uint16_t shootDistance = 10000;
                if (liveClients.find(packet->playerId) != liveClients.end())
                {
                    GameClientData& clientData = gameClientData[packet->playerId];
                    FHitResult hitResult;
                    FVector startLocation = packet->location.asFVector();

                    if (GetWorld()->LineTraceSingleByChannel(hitResult, startLocation,
                        startLocation + clientData.player->GetActorForwardVector() * shootDistance,
                        ECC_Visibility))
                    {
                        shootDistance = hitResult.Distance;
                        if (hitResult.Actor.IsValid())
                        {
                            FString name = hitResult.Actor->GetName();
                            SPACEMMA_DEBUG("Shooter object name: {}", StringCast<ANSICHAR>(*gameClientData[packet->playerId].player->GetName()).Get());
                            SPACEMMA_DEBUG("Shooted object name: {}", StringCast<ANSICHAR>(*name).Get());
                            for (unsigned short otherPlayer : liveClients)
                            {
                                if (otherPlayer != packet->playerId)
                                {
                                    GameClientData& otherPlayerData = gameClientData[otherPlayer];
                                    if (otherPlayerData.player->GetName() == hitResult.Actor->GetName())
                                    {
                                        if (otherPlayerData.player->GetHealth() <= 0.0f)
                                            break;

                                        //TODO:change chardcoded damage value;
                                        uint16_t damage = 20;
                                        sendPacketTo(otherPlayer, S2C_Damage{ S2C_HDamage, {}, otherPlayer, 20 });
                                        SPACEMMA_DEBUG("Shooted object healt points: {}", otherPlayerData.player->GetHealth());

                                        const bool isPlayerDead = otherPlayerData.player->TakeDamage(20.0f);
                                        if (isPlayerDead)
                                        {
                                            SPACEMMA_DEBUG("Shooted object health points: {}", otherPlayerData.player->GetHealth());
                                            clientData.kills += 1;
                                            otherPlayerData.deaths += 1;
                                            SPACEMMA_DEBUG("S2C_UpdateScoreboard: {}, {}", packet->playerId, otherPlayer);
                                            sendPacketToAll(S2C_UpdateScoreboard{S2C_HUpdateScoreboard, packet->playerId, otherPlayer });
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                        else
                        {
                            SPACEMMA_DEBUG("Shooted nothing!");
                        }
                    }
                }
                else
                {
                    SPACEMMA_WARN("Failed to shoot for {}. Player not found!", packet->playerId);
                }
                sendPacketToAllBut(S2C_Shoot{ S2C_HShoot, {}, packet->playerId, shootDistance, packet->location, packet->rotator }, sourceClient);
            }
            else
            {
                dataValid = false;
            }
            break;
        }
        case B2B_HUpdateVelocity:
        {
            B2B_UpdateVelocity* packet = reinterpretPacket<B2B_UpdateVelocity>(span, buffPos);
            if (packet)
            {
                SPACEMMA_DEBUG("B2B_UpdateVelocity: {}, [{},{},{}]",
                    packet->playerId, packet->velocity.x, packet->velocity.y, packet->velocity.z);
                if (liveClients.find(packet->playerId) != liveClients.end())
                {
                    gameClientData[packet->playerId].player->GetCharacterMovement()->Velocity = (packet->velocity.asFVector());
                }
                else
                {
                    SPACEMMA_WARN("Failed to update velocity of {}. Player not found!", packet->playerId);
                }
                sendPacketToAllBut(*packet, sourceClient);
            }
            else
            {
                dataValid = false;
            }
            break;
        }
        case B2B_HRotate:
        {
            B2B_Rotate* packet = reinterpretPacket<B2B_Rotate>(span, buffPos);
            if (packet)
            {
                SPACEMMA_TRACE("B2B_Rotate: {}, [{},{},{}]", packet->playerId,
                    packet->rotator.pitch, packet->rotator.yaw, packet->rotator.roll);
                if (liveClients.find(packet->playerId) != liveClients.end())
                {
                    gameClientData[packet->playerId].player->SetActorRotation(packet->rotator.asFRotator());
                }
                else
                {
                    SPACEMMA_WARN("Failed to change rotation of {}. Player not found!", packet->playerId);
                }
                sendPacketToAllBut(*packet, sourceClient);
            }
            else
            {
                dataValid = false;
            }
            break;
        }
        case B2B_HRopeAttach:
        {
            B2B_RopeAttach* packet = reinterpretPacket<B2B_RopeAttach>(span, buffPos);
            if (packet)
            {
                SPACEMMA_TRACE("B2B_RopeAttach: {}, [{},{},{}]", packet->playerId,
                    packet->attachPosition.x, packet->attachPosition.y, packet->attachPosition.z);
                if (liveClients.find(packet->playerId) != liveClients.end())
                {
                    gameClientData[packet->playerId].player->AttachRope(packet->attachPosition.asFVector(), false);
                }
                else
                {
                    SPACEMMA_WARN("Failed to attach rope for {}. Player not found!", packet->playerId);
                }
                sendPacketToAllBut(*packet, sourceClient);
            }
            else
            {
                dataValid = false;
            }
            break;
        }
        case B2B_HRopeDetach:
        {
            B2B_RopeDetach* packet = reinterpretPacket<B2B_RopeDetach>(span, buffPos);
            if (packet)
            {
                SPACEMMA_TRACE("B2B_RopeDetach: {}", packet->playerId);
                if (liveClients.find(packet->playerId) != liveClients.end())
                {
                    gameClientData[packet->playerId].player->DetachRope(false);
                }
                else
                {
                    SPACEMMA_WARN("Failed to detach rope for {}. Player not found!", packet->playerId);
                }
                sendPacketToAllBut(*packet, sourceClient);
            }
            else
            {
                dataValid = false;
            }
            break;
        }
        case B2B_HDeadPlayer:
        {
            B2B_DeadPlayer* packet = reinterpretPacket<B2B_DeadPlayer>(span, buffPos);
            if (packet)
            {
                SPACEMMA_TRACE("B2B_DeadPlayer: {}", packet->playerId);
                if (liveClients.find(packet->playerId) != liveClients.end())
                {
                    gameClientData[packet->playerId].player->DeadPlayer();
                }
                else
                {
                    SPACEMMA_WARN("Failed to dead player {}. Player not found!", packet->playerId);
                }
                sendPacketToAllBut(*packet, sourceClient);
            }
            else
            {
                dataValid = false;
            }
            break;
        }
        case B2B_HRespawnPlayer:
        {
            B2B_RespawnPlayer* packet = reinterpretPacket<B2B_RespawnPlayer>(span, buffPos);
            if (packet)
            {
                SPACEMMA_TRACE("B2B_RespawnPlayer: {}, [{},{},{}], [{},{},{}]",
                    packet->playerId, packet->location.x, packet->location.y, packet->location.z,
                    packet->rotator.pitch, packet->rotator.yaw, packet->rotator.roll);
                if (liveClients.find(packet->playerId) != liveClients.end())
                {
                    gameClientData[packet->playerId].player->RespawnPlayer(packet->location.asFVector(), packet->rotator.asFRotator());
                }
                else
                {
                    SPACEMMA_WARN("Failed to dead player {}. Player not found!", packet->playerId);
                }
                sendPacketToAllBut(*packet, sourceClient);
            }
            else
            {
                dataValid = false;
            }
            break;
        }
        default:
        {
            SPACEMMA_ERROR("Received invalid packet header of {}! Discarding packet.", header);
            dataValid = false;
            break;
        }
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
        GameClientData& playerData = gameClientData[clientPort];
        playerData.player = actor;
        playerData.recentPosition = actor->GetActorLocation();
        playerData.recentRotation = actor->GetActorRotation();
        S2C_CreatePlayer createPlayerPacket{
            S2C_HCreatePlayer,
            playerData.nickname.length(),
            clientPort,
            playerData.recentPosition,
            playerData.recentRotation,
            (uint16)(RoundTime - currentRoundTime),
            playerData.kills,
            playerData.deaths,
            playerData.nickname
        };
        {
            std::lock_guard lock(liveClientsMutex);
            liveClients.insert(clientPort);
        }
        ByteBuffer* packetBuffer = createPacketBuffer(bufferPool.get(), createPlayerPacket);
        sendToAll(packetBuffer);
        bufferPool->freeBuffer(packetBuffer);
        // tell the player about other players
        for (unsigned short port : liveClients)
        {
            if (port != clientPort)
            {
                GameClientData& otherPlayerData = gameClientData[port];
                createPlayerPacket = {
                    S2C_HCreatePlayer,
                    static_cast<uint8_t>(otherPlayerData.nickname.length()),
                    port,
                    otherPlayerData.player->GetActorLocation(),
                    otherPlayerData.player->GetActorRotation(),
                    (uint16)(RoundTime - currentRoundTime),
                    playerData.kills,
                    playerData.deaths,
                    otherPlayerData.nickname
                };
                packetBuffer = createPacketBuffer(bufferPool.get(), createPlayerPacket);
                sendTo(clientPort, packetBuffer);
                bufferPool->freeBuffer(packetBuffer);
            }
        }
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
    if (liveClients.find(client) != liveClients.end())
    {
        GameClientData& playerData = gameClientData[client];
        sendPacketToAll(S2C_PlayerMovement{ S2C_HPlayerMovement, {}, client, playerData.player->GetActorLocation(),
                           playerData.player->GetActorRotation(), playerData.player->GetCharacterMovement()->Velocity });
    }
    else
    {
        SPACEMMA_WARN("Failed to broadcast movement of {}. Player not found.", client);
    }
}

void AGameServer::broadcastMovingPlayers()
{
    //std::lock_guard lock(liveClientsMutex);
    for (unsigned short port : liveClients)
    {
        GameClientData& playerData = gameClientData[port];
        FVector location = playerData.player->GetActorLocation();
        FRotator rotation = playerData.player->GetActorRotation();
        bool recentlyMoved = playerData.recentlyMoving;
        bool currentlyMoved =
            !playerData.player->GetCharacterMovement()->Velocity.IsNearlyZero() ||
            //!playerPair->second->GetRotationVector().IsNearlyZero() ||
            location != playerData.recentPosition ||
            rotation != playerData.recentRotation;
        if (recentlyMoved || currentlyMoved)
        {
            playerData.recentPosition = location;
            playerData.recentRotation = rotation;
            broadcastPlayerMovement(port);
            playerData.recentlyMoving = currentlyMoved;
        }
    }
}

bool AGameServer::isClientLive(unsigned short client)
{
    //std::lock_guard lock(liveClientsMutex);
    return liveClients.find(client) != liveClients.end();
}

void AGameServer::handleRoundTimer(float deltaTime)
{
    currentRoundTime += deltaTime;
    if (currentRoundTime > RoundTime)
    {
        currentRoundTime = 0;
        for (auto &clientData : gameClientData)
        {
            clientData.second.deaths = 0;
            clientData.second.kills = 0;
        }
        
        sendPacketToAll(S2C_StartRound{ S2C_HStartRound, {}, (uint16_t) RoundTime });
    }
}

void AGameServer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (isServerRunning())
    {
        handlePlayerAwaitingSpawn();
        processAllPendingPackets();
        handlePendingDisconnect();
        handleRoundTimer(DeltaTime);
        currentMovementUpdateDelta += DeltaTime;
        if (currentMovementUpdateDelta >= movementUpdateDelta)
        {
            currentMovementUpdateDelta -= movementUpdateDelta;
            broadcastMovingPlayers();
        }
    }
}

