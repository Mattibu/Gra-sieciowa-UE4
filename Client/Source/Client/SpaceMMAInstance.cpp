#include "SpaceMMAInstance.h"
#include "Client/Server/SpaceLog.h"

void USpaceMMAInstance::Initialize()
{
    switch (Initialization)
    {
        case LevelInitialization::Client:
        {
            SPACEMMA_DEBUG("Initializing client ({}, {}, {})...", StringCast<ANSICHAR>(*ServerIpAddress).Get(), ServerPort, StringCast<ANSICHAR>(*Nickname).Get());
            FActorSpawnParameters params{};
            params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
            AGameClient* client = GetWorld()->SpawnActor<AGameClient>(ClientBP, FVector{}, FRotator{}, FActorSpawnParameters{});
            AShooterPlayer* player = GetWorld()->SpawnActor<AShooterPlayer>(PlayerBP, FVector{}, FRotator{}, params);
            player->AutoPossessPlayer = EAutoReceiveInput::Type::Player0;
            player->AutoPossessAI = EAutoPossessAI::Disabled;
            player->SetGameClient(client);
            client->ServerIpAddress = ServerIpAddress;
            client->ServerPort = ServerPort;
            client->ClientPawn = player;
            client->startConnecting();
        }
        break;
        case LevelInitialization::Server:
        {
            SPACEMMA_DEBUG("Initializing server ({}, {}, {})...", StringCast<ANSICHAR>(*ServerIpAddress).Get(), ServerPort, MaxClients);
            AGameServer* server = GetWorld()->SpawnActor<AGameServer>(ServerBP, FVector{}, FRotator{}, FActorSpawnParameters{});
            server->ServerIpAddress = ServerIpAddress;
            server->ServerPort = ServerPort;
            server->startServer();
        }
        break;
        default:
            SPACEMMA_WARN("Unhandled level initialization mode.");
            break;
    }
    Initialization = LevelInitialization::None;
}
