#include "GameClient.h"
#include "Client/Server/WinThread.h"
#include "Client/Shared/Packets.h"
#include "Engine/World.h"

using namespace spacemma;

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
    closeConnection();
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
        SPACEMMA_DEBUG("Interrupting client threads...");
        bool conn = tcpClient->isConnected();
        if (conn)
        {
            sendThread->interrupt();
            receiveThread->interrupt();
        }
        SPACEMMA_DEBUG("Closing tcpClient...");
        tcpClient->close();
        SPACEMMA_DEBUG("Joining threads...");
        if (conn)
        {
            sendThread->join();
            receiveThread->join();
            delete sendThread;
            delete receiveThread;
        }
        SPACEMMA_DEBUG("Resetting tcpClient...");
        tcpClient.reset();
        SPACEMMA_DEBUG("Resetting bufferPool...");
        bufferPool.reset();
        playerId = 0;
        return true;
    }
    SPACEMMA_DEBUG("Resetting bufferPool...");
    bufferPool.reset();
    playerId = 0;
    return false;
}

void AGameClient::shoot(FVector location, FRotator rotator)
{
    if (isConnectedAndIdentified())
    {
        sendPacket(B2B_Shoot{ B2B_HShoot, {}, playerId, location, rotator });
    }
}

void AGameClient::changeSpeed(FVector speedVector)
{
    if (isConnectedAndIdentified())
    {
        sendPacket(B2B_ChangeSpeed{ B2B_HChangeSpeed, {}, playerId, speedVector });
    }
}

void AGameClient::rotate(FVector rotationVector)
{
    if (isConnectedAndIdentified())
    {
        sendPacket(B2B_Rotate{ B2B_HRotate, {}, playerId, rotationVector });
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

void AGameClient::threadConnect(gsl::not_null<Thread*> thread, void* client)
{
    AGameClient* clt = reinterpret_cast<AGameClient*>(client);
    SPACEMMA_DEBUG("Starting threadConnect...");
    do
    {
        if (clt->tcpClient->connect(StringCast<ANSICHAR>(*clt->ServerIpAddress).Get(), clt->ServerPort))
        {
            clt->receiveThread = new WinThread();
            clt->sendThread = new WinThread();
            clt->receiveThread->run(threadReceive, clt);
            clt->sendThread->run(threadSend, clt);
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
    Header header = static_cast<Header>(*buffer->getPointer());
    switch (header)
    {
        case S2C_HProvidePlayerId:
        {
            S2C_ProvidePlayerId* packet = reinterpretPacket<S2C_ProvidePlayerId>(buffer);
            if (packet)
            {
                SPACEMMA_DEBUG("S2C_ProvidePlayerId: {}", packet->playerId);
                this->playerId = packet->playerId;
            }
            break;
        }
        case S2C_HCreatePlayer:
        {
            S2C_CreatePlayer* packet = reinterpretPacket<S2C_CreatePlayer>(buffer);
            if (packet)
            {
                SPACEMMA_DEBUG("S2C_CreatePlayer: {}, [{},{},{}], [{},{},{}]", packet->playerId,
                               packet->location.x, packet->location.y, packet->location.z,
                               packet->rotator.pitch, packet->rotator.yaw, packet->rotator.roll);
                AActor* actor = GetWorld()->SpawnActor<AActor>(PlayerBP, packet->location.asFVector(), packet->rotator.asFRotator(), FActorSpawnParameters{});
                otherPlayers.emplace(packet->playerId, actor);
            }
            break;
        }
        case S2C_HDestroyPlayer:
        {
            S2C_DestroyPlayer* packet = reinterpretPacket<S2C_DestroyPlayer>(buffer);
            if (packet)
            {
                SPACEMMA_DEBUG("S2C_DestroyPlayer: {}", packet->playerId);
                const std::map<unsigned short, AActor*>::iterator pair = otherPlayers.find(packet->playerId);
                if (pair != otherPlayers.end())
                {
                    pair->second->Destroy();
                    otherPlayers.erase(pair);
                }
            }
            break;
        }
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
        case S2C_HPlayerMovement:
        {
            S2C_PlayerMovement* packet = reinterpretPacket<S2C_PlayerMovement>(buffer);
            if (packet)
            {
                SPACEMMA_DEBUG("S2C_PlayerMovement: {}, [{},{},{}], [{},{},{}], {}, [{},{},{}]", packet->playerId,
                               packet->location.x, packet->location.y, packet->location.z,
                               packet->rotator.pitch, packet->rotator.yaw, packet->rotator.roll,
                               packet->speedValue, packet->speedVector.x, packet->speedVector.y, packet->speedVector.z);
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
        case S2C_HRopeFailed:
        {
            S2C_RopeFailed* packet = reinterpretPacket<S2C_RopeFailed>(buffer);
            if (packet)
            {
                SPACEMMA_DEBUG("S2C_RopeFailed: {}, {}", packet->playerId, packet->ropeCooldown);
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

void AGameClient::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    processPendingPacket();
}

