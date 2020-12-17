#include "GameClient.h"
#include "Client/Server/WinThread.h"
#include "Client/Shared/Packets.h"

using namespace spacemma;

AGameClient::AGameClient()
{
    PrimaryActorTick.bCanEverTick = true;

}

void AGameClient::BeginPlay()
{
    Super::BeginPlay();
    connectThread = new WinThread();
    connectThread->run(threadConnect, this);
}

void AGameClient::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    connectThread->interrupt();
    connectThread->join();
    delete connectThread;
    if (tcpClient.isConnected())
    {
        sendThread->interrupt();
        receiveThread->interrupt();
        processPacketsThread->interrupt();
        tcpClient.close();
        sendThread->join();
        receiveThread->join();
        processPacketsThread->join();
        delete sendThread;
        delete receiveThread;
        delete processPacketsThread;
    }
}

void AGameClient::threadConnect(gsl::not_null<Thread*> thread, void* client)
{
    AGameClient* clt = reinterpret_cast<AGameClient*>(client);
    do
    {
        std::lock_guard lock(clt->connectionMutex);
        if (clt->tcpClient.connect("127.0.0.1", 4444))
        {
            clt->receiveThread = new WinThread();
            clt->sendThread = new WinThread();
            clt->processPacketsThread = new WinThread();
            clt->receiveThread->run(threadReceive, clt);
            clt->sendThread->run(threadSend, clt);
            clt->processPacketsThread->run(threadProcessPackets, clt);
        } else
        {
            SERVER_ERROR("Failed to connect to server");
        }
    } while (!thread->isInterrupted() && !clt->tcpClient.isConnected());
}

void AGameClient::threadReceive(gsl::not_null<Thread*> thread, void* client)
{
    AGameClient* clt = reinterpret_cast<AGameClient*>(client);
    do
    {
        ByteBuffer* buffer = clt->tcpClient.receive();
        if (buffer)
        {
            std::lock_guard lock(clt->receiveMutex);
            clt->receivedPackets.push_back(buffer);
        }
    } while (!thread->isInterrupted());
}

void AGameClient::threadSend(gsl::not_null<Thread*> thread, void* client)
{
    AGameClient* clt = reinterpret_cast<AGameClient*>(client);
    ByteBuffer* toSend{ nullptr };
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
            clt->tcpClient.send(toSend);
            clt->bufferPool.freeBuffer(toSend);
            toSend = nullptr;
        }
    } while (!thread->isInterrupted());
}

void AGameClient::threadProcessPackets(gsl::not_null<Thread*> thread, void* client)
{
    AGameClient* clt = reinterpret_cast<AGameClient*>(client);
    ByteBuffer* packet{ nullptr };
    do
    {
        {
            std::lock_guard lock(clt->receiveMutex);
            if (!clt->receivedPackets.empty())
            {
                packet = clt->receivedPackets[0];
                clt->receivedPackets.erase(clt->receivedPackets.begin());
            }
        }
        if (packet)
        {
            clt->processPacket(packet);
            clt->bufferPool.freeBuffer(packet);
            packet = nullptr;
        }
    } while (!thread->isInterrupted());
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
        case S2C_HCreatePlayer:
        {
            S2C_CreatePlayer* packet = reinterpretPacket<S2C_CreatePlayer>(buffer);
            if (packet)
            {
                SERVER_DEBUG("S2C_CreatePlayer: {}, [{},{},{}], [{},{},{}]", packet->playerId,
                             packet->location.x, packet->location.y, packet->location.z,
                             packet->rotator.pitch, packet->rotator.yaw, packet->rotator.roll);
            }
            break;
        }
        case S2C_HDestroyPlayer:
        {
            S2C_DestroyPlayer* packet = reinterpretPacket<S2C_DestroyPlayer>(buffer);
            if (packet)
            {
                SERVER_DEBUG("S2C_DestroyPlayer: {}", packet->playerId);
            }
            break;
        }
        case B2B_HShoot:
        {
            B2B_Shoot* packet = reinterpretPacket<B2B_Shoot>(buffer);
            if (packet)
            {
                SERVER_DEBUG("B2B_Shoot: {}, [{},{},{}], [{},{},{}]",
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
                SERVER_DEBUG("B2B_ChangeSpeed: {}, {}, [{},{},{}]",
                             packet->playerId, packet->speedValue,
                             packet->speedVector.x, packet->speedVector.y, packet->speedVector.z);
            }
            break;
        }
        case B2B_HRotate:
        {
            B2B_Rotate* packet = reinterpretPacket<B2B_Rotate>(buffer);
            if (packet)
            {
                SERVER_DEBUG("B2B_Rotate: {}, [{},{},{}]", packet->playerId,
                             packet->rotationVector.x, packet->rotationVector.y, packet->rotationVector.z);
            }
            break;
        }
        case S2C_HPlayerMovement:
        {
            S2C_PlayerMovement* packet = reinterpretPacket<S2C_PlayerMovement>(buffer);
            if (packet)
            {
                SERVER_DEBUG("S2C_PlayerMovement: {}, [{},{},{}], [{},{},{}], {}, [{},{},{}]", packet->playerId,
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
                SERVER_DEBUG("B2B_RopeAttach: {}, [{},{},{}]", packet->playerId,
                             packet->attachPosition.x, packet->attachPosition.y, packet->attachPosition.z);
            }
            break;
        }
        case S2C_HRopeFailed:
        {
            S2C_RopeFailed* packet = reinterpretPacket<S2C_RopeFailed>(buffer);
            if (packet)
            {
                SERVER_DEBUG("S2C_RopeFailed: {}, {}", packet->playerId, packet->ropeCooldown);
            }
            break;
        }
        case B2B_HRopeDetach:
        {
            B2B_RopeDetach* packet = reinterpretPacket<B2B_RopeDetach>(buffer);
            if (packet)
            {
                SERVER_DEBUG("B2B_RopeDetach: {}", packet->playerId);
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

void AGameClient::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

}

