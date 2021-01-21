#include "GameClient.h"
#include "Client/Server/WinThread.h"
#include "Client/Shared/Packets.h"
#include "Client/Server/WinTCPClient.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

using namespace spacemma;
using namespace spacemma::packets;

AGameClient::AGameClient()
{
    PrimaryActorTick.bCanEverTick = true;

}

void AGameClient::BeginPlay()
{
    Super::BeginPlay();
}

void AGameClient::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    switch (EndPlayReason)
    {
        case EEndPlayReason::Type::EndPlayInEditor:
        case EEndPlayReason::Type::RemovedFromWorld:
            closeConnection();
            break;
    }
    Super::EndPlay(EndPlayReason);
}

bool AGameClient::startConnecting()
{
    std::lock_guard lock(connectionMutex);
    if (connectThread || tcpClient)
    {
        SPACEMMA_WARN("Attempted to start connecting while connecting or when client was already created!");
        return false;
    }
    bufferPool = std::make_unique<BufferPool>(1024 * 1024 * 1024);
    tcpClient = std::make_unique<WinTCPClient>(*bufferPool);
    connectThread = new WinThread();
    connectThread->run(threadConnect, this);
    return true;
}

bool AGameClient::isConnecting()
{
    std::lock_guard lock(connectionMutex);
    return connectThread && !tcpClient->isConnected();
}

bool AGameClient::isConnected()
{
    return tcpClient && tcpClient->isConnected();
}

bool AGameClient::isIdentified() const
{
    return playerId != 0;
}

bool AGameClient::isConnectedAndIdentified()
{
    return isIdentified() && isConnected();
}

bool AGameClient::closeConnection()
{
    std::lock_guard lock(connectionMutex);
    SPACEMMA_DEBUG("Closing connection...");
    if (connectThread)
    {
        SPACEMMA_DEBUG("Joining connectThread...");
        connectThread->interrupt();
        connectThread->join();
        delete connectThread;
        connectThread = nullptr;
    }
    if (tcpClient)
    {
        if (sendThread)
        {
            SPACEMMA_DEBUG("Interrupting sendThread...");
            sendThread->interrupt();
        }
        if (receiveThread)
        {
            SPACEMMA_DEBUG("Interrupting receiveThread...");
            receiveThread->interrupt();
        }
        SPACEMMA_DEBUG("Closing tcpClient...");
        tcpClient->close();
        SPACEMMA_DEBUG("Joining threads...");
        if (sendThread)
        {
            sendThread->join();
            delete sendThread;
        }
        if (receiveThread)
        {
            receiveThread->join();
            delete receiveThread;
        }
        SPACEMMA_DEBUG("Resetting tcpClient...");
        tcpClient.reset();
        SPACEMMA_DEBUG("Resetting bufferPool...");
        bufferPool.reset();
        otherPlayers.clear();
        playerId = 0;
        return true;
    }
    return false;
}

void AGameClient::shoot(FVector shootingPosition)
{
    if (isConnectedAndIdentified())
    {
        sendPacket(C2S_Shoot{ C2S_HShoot, {}, playerId, shootingPosition, ClientPawn->GetActorRotation() });
    }
}

void AGameClient::attachRope(FVector attachPosition)
{
    if (isConnectedAndIdentified())
    {
        sendPacket(B2B_RopeAttach{ B2B_HRopeAttach, {}, playerId, attachPosition });
    }
}

void AGameClient::detachRope()
{
    if (isConnectedAndIdentified())
    {
        sendPacket(B2B_RopeDetach{ B2B_HRopeDetach, {}, playerId });
    }
}

void AGameClient::updateVelocity(FVector velocity)
{
    if (isConnectedAndIdentified())
    {
        sendPacket(B2B_UpdateVelocity{ B2B_HUpdateVelocity, {}, playerId, velocity });
    }
}

void AGameClient::updateRotation()
{
    if (isConnectedAndIdentified())
    {
        sendPacket(B2B_Rotate{ B2B_HRotate, {}, playerId, ClientPawn->GetActorRotation() });
    }
}

void AGameClient::dead()
{
    if (isConnectedAndIdentified())
    {
        sendPacket(B2B_DeadPlayer{ B2B_HDeadPlayer, {}, playerId });
    }
}

void AGameClient::respawn(FVector position, FRotator rotator)
{
    if (isConnectedAndIdentified())
    {
        sendPacket(B2B_RespawnPlayer{ B2B_HRespawnPlayer, {}, playerId, position, rotator });
    }
}


void AGameClient::threadConnect(gsl::not_null<Thread*> thread, void* client)
{
    AGameClient* clt = reinterpret_cast<AGameClient*>(client);
    SPACEMMA_DEBUG("Starting threadConnect...");
    do
    {
        if (clt->tcpClient->connect(StringCast<ANSICHAR>(*clt->ServerIpAddress).Get(), clt->ServerPort))
        {
            FString mapName = clt->GetWorld()->GetName();
            C2S_InitConnection initPacket{
                C2S_HInitConnection,
                static_cast<uint8_t>(mapName.Len()),
                static_cast<uint8_t>(clt->Nickname.Len()),
                {},
                StringCast<ANSICHAR>(*mapName).Get(),
                       StringCast<ANSICHAR>(*clt->Nickname).Get()
            };
            ByteBuffer* initPacketBuff = createPacketBuffer(clt->bufferPool.get(), initPacket);
            clt->receiveThread = new WinThread();
            clt->sendThread = new WinThread();
            clt->receiveThread->run(threadReceive, clt);
            clt->sendThread->run(threadSend, clt);
            SPACEMMA_DEBUG("Sending C2S_InitConnection ('{}', '{}')...", initPacket.mapName, initPacket.nickname);
            clt->tcpClient->send(initPacketBuff);
            clt->bufferPool->freeBuffer(initPacketBuff);
        } else
        {
            SPACEMMA_ERROR("Failed to connect to server");
        }
    } while (!thread->isInterrupted() && !clt->tcpClient->isConnected());
    SPACEMMA_DEBUG("Stopping threadConnect...");
}

void AGameClient::threadReceive(gsl::not_null<Thread*> thread, void* client)
{
    AGameClient* clt = reinterpret_cast<AGameClient*>(client);
    SPACEMMA_DEBUG("Starting threadReceive...");
    do
    {
        ByteBuffer* buffer = clt->tcpClient->receive();
        if (buffer)
        {
            std::lock_guard lock(clt->receiveMutex);
            clt->receivedPackets.push_back(buffer);
        } else if (!clt->tcpClient->isConnected())
        {
            SPACEMMA_ERROR("Connection lost.");
            break;
        }
    } while (!thread->isInterrupted());
    SPACEMMA_DEBUG("Stopping threadReceive...");
}

void AGameClient::threadSend(gsl::not_null<Thread*> thread, void* client)
{
    AGameClient* clt = reinterpret_cast<AGameClient*>(client);
    ByteBuffer* toSend{ nullptr };
    SPACEMMA_DEBUG("Starting threadSend...");
    do
    {
        {
            std::lock_guard lock(clt->sendMutex);
            if (!clt->toSendPackets.empty())
            {
                toSend = clt->toSendPackets[0];
                clt->toSendPackets.erase(clt->toSendPackets.begin());
            }
        }
        if (toSend)
        {
            bool sent = clt->tcpClient->send(toSend);
            //SPACEMMA_DEBUG("Sent packet {}!", *reinterpret_cast<uint8_t*>(toSend->getPointer()));
            clt->bufferPool->freeBuffer(toSend);
            if (!sent && !clt->tcpClient->isConnected())
            {
                SPACEMMA_ERROR("Connection lost.");
                break;
            }
            toSend = nullptr;
        }
    } while (!thread->isInterrupted());
    SPACEMMA_DEBUG("Stopping threadSend...");
}

void AGameClient::send(gsl::not_null<ByteBuffer*> packet)
{
    std::lock_guard lock(sendMutex);
    toSendPackets.push_back(packet);
}

void AGameClient::processPacket(ByteBuffer* buffer)
{
    gsl::span<uint8_t> span = buffer->getSpan();
    size_t buffPos = 0;
    bool dataValid = true;
    do
    {
        Header header = static_cast<Header>(span[buffPos]);
        switch (header)
        {
            case S2C_HProvidePlayerId:
            {
                S2C_ProvidePlayerId* packet = reinterpretPacket<S2C_ProvidePlayerId>(span, buffPos);
                if (packet)
                {
                    SPACEMMA_DEBUG("S2C_ProvidePlayerId: {}", packet->playerId);
                    this->playerId = packet->playerId;
                } else
                {
                    dataValid = false;
                }
                break;
            }
            case S2C_HCreatePlayer:
            {
                S2C_CreatePlayer packet;
                if (reinterpretPacket(span, buffPos, packet))
                {
                    SPACEMMA_DEBUG("S2C_CreatePlayer: {}, [{},{},{}], [{},{},{}], '{}', {}, {}", packet.playerId,
                                   packet.location.x, packet.location.y, packet.location.z,
                                   packet.rotator.pitch, packet.rotator.yaw, packet.rotator.roll,
                                   packet.nickname, packet.kills, packet.deaths);
                    bool valid = true;
                    FVector locVec{ packet.location.asFVector() };
                    if (packet.playerId == playerId)
                    {
                        SPACEMMA_DEBUG("Adjusting self-position of {} (S2C_CreatePlayer)...", playerId);
                        ClientPawn->SetActorLocation(locVec);
                        ClientPawn->SetActorRotation(packet.rotator.asFRotator());
                        ClientPawn->StartRound(packet.roundTime);
                    } else if (otherPlayers.find(packet.playerId) != otherPlayers.end())
                    {
                        valid = false;
                        SPACEMMA_ERROR("Attempted to spawn player {} which is already up!", packet.playerId);
                    } else
                    {
                        SPACEMMA_DEBUG("Spawning player {} ({})...", packet.playerId, playerId);
                        FActorSpawnParameters params{};
                        params.Name = FName(FString::Printf(TEXT("Player #%d"), static_cast<int32>(packet.playerId)));
                        params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
                        AShooterPlayer* actor =
                            GetWorld()->SpawnActor<AShooterPlayer>(PlayerBP, locVec, packet.rotator.asFRotator(), params);
                        if (actor == nullptr)
                        {
                            valid = false;
                            SPACEMMA_ERROR("Failed to create player {}!", packet.playerId);
                        } else
                        {
                            actor->InitiateClientPlayer();
                            ClientPawn->AddPlayerToScoreboard(FString(packet.nickname.c_str()), packet.kills, packet.deaths);
                            otherPlayers.insert({ packet.playerId, {actor, packet.nickname, packet.kills, packet.deaths } });
                        }
                    }
                    if (valid)
                    {
                        recentPosData.emplace(packet.playerId, RecentPosData{ locVec, locVec, FVector::ZeroVector, FVector::ZeroVector, 0.0f });
                    }
                } else
                {
                    dataValid = false;
                }
                break;
            }
            case S2C_HDestroyPlayer:
            {
                S2C_DestroyPlayer* packet = reinterpretPacket<S2C_DestroyPlayer>(span, buffPos);
                if (packet)
                {
                    SPACEMMA_DEBUG("S2C_DestroyPlayer: {}", packet->playerId);
                    const std::map<unsigned short, OtherPlayerData>::iterator pair = otherPlayers.find(packet->playerId);
                    const std::map<unsigned short, RecentPosData>::iterator pair1 = recentPosData.
                        find(packet->playerId);
                    if (pair1 != recentPosData.end())
                    {
                        recentPosData.erase(pair1);
                    }
                    if (pair != otherPlayers.end())
                    {
                        SPACEMMA_DEBUG("Destroying player {} '{}' ({})...", packet->playerId, pair->second.nickname, playerId);
                        pair->second.player->Destroy();
                        ClientPawn->RemovePlayerFromScoreboard(FString(pair->second.nickname.c_str()));
                        otherPlayers.erase(pair);
                    }
                } else
                {
                    dataValid = false;
                }
                break;
            }
            case S2C_HDamage:
            {
                S2C_Damage* packet = reinterpretPacket<S2C_Damage>(span, buffPos);
                if (packet)
                {
                    SPACEMMA_DEBUG("S2C_Damage: {}, {}",
                                   packet->playerId, packet->damage);
                    if (packet->playerId == playerId)
                    {
                        ClientPawn->ReceiveDamage(packet->damage);
                        break;
                    }
                } else
                {
                    dataValid = false;
                }
                break;
            }
            case S2C_HShoot:
            {
                S2C_Shoot* packet = reinterpretPacket<S2C_Shoot>(span, buffPos);
                if (packet)
                {
                    SPACEMMA_DEBUG("S2C_Shoot: {}, {}, [{},{},{}], [{},{},{}]",
                                   packet->playerId, packet->distance, packet->location.x, packet->location.y, packet->location.z,
                                   packet->rotator.pitch, packet->rotator.yaw, packet->rotator.roll);
                    if (packet->playerId == playerId)
                    {
                        break;
                    }
                    const std::map<unsigned short, OtherPlayerData>::iterator pair = otherPlayers.find(packet->playerId);
                    if (pair != otherPlayers.end())
                    {
                        pair->second.player->Shoot(packet->location.asFVector(), packet->rotator.asFRotator(), packet->distance);
                        pair->second.player->SetActorRotation(packet->rotator.asFRotator());
                    } else
                    {
                        SPACEMMA_WARN("Unable to create shoot effect for {} ({}). Player not found!", packet->playerId, playerId);
                    }
                } else
                {
                    dataValid = false;
                }
                break;
            }
            case S2C_HStartRound:
            {
                S2C_StartRound* packet = reinterpretPacket<S2C_StartRound>(span, buffPos);
                if (packet)
                {
                    SPACEMMA_DEBUG("S2C_StartRound: {}",
                                   packet->roundTime);
                    kills = 0;
                    deaths = 0;
                    for (auto& otherPlayer : otherPlayers)
                    {
                        otherPlayer.second.deaths = 0;
                        otherPlayer.second.kills = 0;
                    }
                    ClientPawn->StartRound(packet->roundTime);
                } else
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
                    SPACEMMA_TRACE("B2B_UpdateVelocity: {}, [{},{},{}]",
                                   packet->playerId, packet->velocity.x, packet->velocity.y, packet->velocity.z);
                    if (packet->playerId == playerId)
                    {
                        ClientPawn->GetCharacterMovement()->Velocity = packet->velocity.asFVector();
                    } else
                    {
                        const std::map<unsigned short, OtherPlayerData>::iterator pair = otherPlayers.find(packet->playerId);
                        if (pair != otherPlayers.end())
                        {
                            pair->second.player->GetCharacterMovement()->Velocity = packet->velocity.asFVector();
                        } else
                        {
                            SPACEMMA_WARN("Unable to adjust velocity of {} ({}). Player not found!", packet->playerId, playerId);
                        }
                    }
                } else
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
                    if (packet->playerId == playerId)
                    {
                        ClientPawn->SetActorRotation(packet->rotator.asFRotator());
                    } else
                    {
                        const std::map<unsigned short, OtherPlayerData>::iterator pair = otherPlayers.find(packet->playerId);
                        if (pair != otherPlayers.end())
                        {
                            pair->second.player->SetActorRotation(packet->rotator.asFRotator());
                        } else
                        {
                            SPACEMMA_WARN("Unable to adjust rotation vector of {} ({}). Player not found!", packet->playerId, playerId);
                        }
                    }
                } else
                {
                    dataValid = false;
                }
                break;
            }
            case S2C_HPlayerMovement:
            {
                S2C_PlayerMovement* packet = reinterpretPacket<S2C_PlayerMovement>(span, buffPos);
                if (packet)
                {
                    SPACEMMA_TRACE("S2C_PlayerMovement: {}, [{},{},{}], [{},{},{}], [{},{},{}]", packet->playerId,
                                   packet->location.x, packet->location.y, packet->location.z,
                                   packet->rotator.pitch, packet->rotator.yaw, packet->rotator.roll,
                                   packet->velocity.x, packet->velocity.y, packet->velocity.z);
                    if (packet->playerId == playerId)
                    {
                        //ClientPawn->SetActorRotation(packet->rotator.asFRotator());
                        ClientPawn->SetActorLocation(packet->location.asFVector());
                        ClientPawn->GetCharacterMovement()->Velocity = packet->velocity.asFVector();
                        //auto& posDelta = recentPosData[playerId];
                        //posDelta.prevPos = ClientPawn->GetActorLocation();
                        //posDelta.nextPos = packet->location.asFVector();
                        //posDelta.prevVelocity = ClientPawn->GetCharacterMovement()->Velocity;
                        //posDelta.nextVelocity = packet->velocity.asFVector();
                        //posDelta.timePassed = 0.0f;
                    } else
                    {
                        const std::map<unsigned short, OtherPlayerData>::iterator pair = otherPlayers.find(packet->playerId);
                        if (pair != otherPlayers.end())
                        {
                            pair->second.player->SetActorRotation(packet->rotator.asFRotator());
                            pair->second.player->SetActorLocation(packet->location.asFVector());
                            pair->second.player->GetCharacterMovement()->Velocity = packet->velocity.asFVector();
                            //auto& posDelta = recentPosData[packet->playerId];
                            //posDelta.prevPos = pair->second->GetActorLocation();
                            //posDelta.nextPos = packet->location.asFVector();
                            //posDelta.prevVelocity = pair->second->GetCharacterMovement()->Velocity;
                            //posDelta.nextVelocity = packet->velocity.asFVector();
                            //posDelta.timePassed = 0.0f;
                        } else
                        {
                            SPACEMMA_WARN("Unable to update movement of {} ({}). Player not found!", packet->playerId, playerId);
                        }
                    }
                } else
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
                    if (packet->playerId == playerId)
                    {
                        ClientPawn->AttachRope(packet->attachPosition.asFVector(), false);
                    } else
                    {
                        const std::map<unsigned short, OtherPlayerData>::iterator pair = otherPlayers.find(packet->playerId);
                        if (pair != otherPlayers.end())
                        {
                            pair->second.player->AttachRope(packet->attachPosition.asFVector(), false);
                        } else
                        {
                            SPACEMMA_WARN("Unable to attach rope for player {} ({}). Player not found!", packet->playerId, playerId);
                        }
                    }
                } else
                {
                    dataValid = false;
                }
                break;
            }
            case S2C_HRopeFailed:
            {
                S2C_RopeFailed* packet = reinterpretPacket<S2C_RopeFailed>(span, buffPos);
                if (packet)
                {
                    SPACEMMA_TRACE("S2C_RopeFailed: {}, {}", packet->playerId, packet->ropeCooldown);
                    if (packet->playerId == playerId)
                    {
                        ClientPawn->SetRopeCooldown(packet->ropeCooldown, false);
                    } else
                    {
                        const std::map<unsigned short, OtherPlayerData>::iterator pair = otherPlayers.find(packet->playerId);
                        if (pair != otherPlayers.end())
                        {
                            pair->second.player->SetRopeCooldown(packet->ropeCooldown, false);
                        } else
                        {
                            SPACEMMA_WARN("Unable to update rope cooldown for player {} ({}). Player not found!", packet->playerId, playerId);
                        }
                    }
                } else
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
                    if (packet->playerId == playerId)
                    {
                        ClientPawn->DetachRope(false);
                    } else
                    {
                        const std::map<unsigned short, OtherPlayerData>::iterator pair = otherPlayers.find(packet->playerId);
                        if (pair != otherPlayers.end())
                        {
                            pair->second.player->DetachRope(false);
                        } else
                        {
                            SPACEMMA_WARN("Unable to detach rope for player {} ({}). Player not found!", packet->playerId, playerId);
                        }
                    }
                } else
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
                    SPACEMMA_DEBUG("B2B_DeadPlayer: {}",
                                   packet->playerId);
                    if (packet->playerId == playerId)
                    {
                        break;
                    }
                    const std::map<unsigned short, OtherPlayerData>::iterator pair = otherPlayers.find(packet->playerId);
                    if (pair != otherPlayers.end())
                    {
                        pair->second.player->DeadPlayer();
                    } else
                    {
                        SPACEMMA_WARN("Unable to dead {} ({}). Player not found!", packet->playerId, playerId);
                    }
                } else
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
                    SPACEMMA_DEBUG("B2B_RespawnPlayer: {}", packet->playerId);
                    if (packet->playerId == playerId)
                    {
                        break;
                    }
                    const std::map<unsigned short, OtherPlayerData>::iterator pair = otherPlayers.find(packet->playerId);
                    if (pair != otherPlayers.end())
                    {
                        recentPosData[packet->playerId] = RecentPosData{ packet->location.asFVector(), packet->location.asFVector(), FVector::ZeroVector, FVector::ZeroVector, 0.0f };
                        pair->second.player->RespawnPlayer(packet->location.asFVector(), packet->rotator.asFRotator());
                    } else
                    {
                        SPACEMMA_WARN("Unable to respawn {} ({}). Player not found!", packet->playerId, playerId);
                    }
                } else
                {
                    dataValid = false;
                }
                break;
            }
            case S2C_HUpdateScoreboard:
            {
                S2C_UpdateScoreboard* packet = reinterpretPacket<S2C_UpdateScoreboard>(span, buffPos);
                if (packet)
                {
                    SPACEMMA_DEBUG("Game Client received: S2C_UpdateScoreboard: {}, {}", packet->killerPlayerId, packet->killedPlayerId);
                    FString killerNickname, victimNickname;
                    if (packet->killerPlayerId == playerId)
                    {
                        killerNickname = Nickname;
                        kills += 1;
                        ClientPawn->UpdatePlayerOnScoreboard(Nickname, kills, deaths);
                        const auto& pairIt = otherPlayers.find(packet->killedPlayerId);
                        if (pairIt != otherPlayers.end())
                        {
                            victimNickname = pairIt->second.nickname.c_str();
                            auto& otherPlayer = pairIt->second;
                            otherPlayer.deaths += 1;
                            ClientPawn->UpdatePlayerOnScoreboard(FString(otherPlayer.nickname.c_str()), otherPlayer.kills, otherPlayer.deaths);
                        }
                    } else if (packet->killedPlayerId == playerId)
                    {
                        victimNickname = Nickname;
                        deaths += 1;
                        ClientPawn->UpdatePlayerOnScoreboard(Nickname, kills, deaths);
                        const auto& pairIt = otherPlayers.find(packet->killerPlayerId);
                        if (pairIt != otherPlayers.end())
                        {
                            killerNickname = pairIt->second.nickname.c_str();
                            auto& otherPlayer = pairIt->second;
                            otherPlayer.kills += 1;
                            ClientPawn->UpdatePlayerOnScoreboard(FString(otherPlayer.nickname.c_str()), otherPlayer.kills, otherPlayer.deaths);
                        }
                    }
                    if (killerNickname.IsEmpty())
                    {
                        const auto& pairIt = otherPlayers.find(packet->killerPlayerId);
                        if (pairIt != otherPlayers.end())
                        {
                            killerNickname = pairIt->second.nickname.c_str();
                        }
                    }
                    if (victimNickname.IsEmpty())
                    {
                        const auto& pairIt = otherPlayers.find(packet->killedPlayerId);
                        if (pairIt != otherPlayers.end())
                        {
                            victimNickname = pairIt->second.nickname.c_str();
                        }
                    }
                    ClientPawn->AddKillNotification(killerNickname, victimNickname);
                } else
                {
                    dataValid = false;
                }
                break;
            }
            case S2C_HInvalidNickname:
            {
                SPACEMMA_ERROR("S2C_InvalidNickname");
                break;
            }
            case S2C_HInvalidMap:
            {
                SPACEMMA_ERROR("S2C_InvalidMap");
                break;
            }
            case S2C_HInvalidData:
            {
                SPACEMMA_ERROR("S2C_InvalidData");
                break;
            }
            default:
            {
                SPACEMMA_ERROR("Received invalid packet header of {}! Discarding packet.", header);
                dataValid = false;
                break;
            }
        }
    } while (dataValid && buffPos < span.size());
}

void AGameClient::processPendingPacket()
{
    ByteBuffer* packet = nullptr;
    {
        std::lock_guard lock(receiveMutex);
        if (!receivedPackets.empty())
        {
            packet = receivedPackets[0];
            receivedPackets.erase(receivedPackets.begin());
        }
    }
    if (packet)
    {
        processPacket(packet);
        bufferPool->freeBuffer(packet);
    }
}

void AGameClient::interpolateMovement(float deltaTime)
{
    for (std::map<unsigned short, RecentPosData>::value_type& pair : recentPosData)
    {
        AShooterPlayer* player = pair.first == playerId ? ClientPawn : nullptr;
        if (!player)
        {
            const std::map<unsigned short, OtherPlayerData>::iterator pair1 = otherPlayers.find(pair.first);
            if (pair1 != otherPlayers.end())
            {
                player = pair1->second.player;
            }
        }
        if (player)
        {
            pair.second.timePassed += deltaTime;
            FVector targetPosition = (pair.second.nextPos - pair.second.prevPos) * pair.second.timePassed * InterpolationStrength + pair.second.prevPos;
            FVector targetVelocity = (pair.second.nextVelocity - pair.second.prevVelocity) * pair.second.timePassed * InterpolationStrength + pair.second.prevVelocity;
            player->SetActorLocation(targetPosition * InterpolationSharpness + player->GetActorLocation() * (1.0f - InterpolationSharpness));
            player->GetCharacterMovement()->Velocity = targetVelocity * InterpolationSharpness + player->GetCharacterMovement()->Velocity * (1.0f - InterpolationSharpness);
        } else
        {
            SPACEMMA_WARN("Player {} not found for movement interpolation!", pair.first);
        }
    }
}

void AGameClient::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (isConnected())
    {
        processPendingPacket();
        //interpolateMovement(DeltaTime);
    }
}

