#include "Menu.h"

#include <WS2tcpip.h>
#include "Kismet/GameplayStatics.h"
#include "Client/SpaceMMAInstance.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

bool UMenu::IsIpValid(FString trimmedIp)
{
    sockaddr_in sa;
    return InetPtonA(AF_INET, StringCast<ANSICHAR>(*trimmedIp).Get(), &sa.sin_addr) > 0;
}

bool UMenu::IsPortValid(FString trimmedPort)
{
    if (!trimmedPort.IsNumeric())
    {
        return false;
    }
    int32 port = FCString::Atoi(*trimmedPort);
    return port >= 1024 && port <= 49151;
}

void UMenu::PrepareServer(FString trimmedIp, FString trimmedPort, FString roundTime)
{
    USpaceMMAInstance* instance = reinterpret_cast<USpaceMMAInstance*>(GetGameInstance());
    instance->Initialization = USpaceMMAInstance::LevelInitialization::Server;
    instance->ServerIpAddress = trimmedIp;
    instance->ServerPort = FCString::Atoi(*trimmedPort);
    instance->MaxClients = 8;
    instance->RoundTime = FCString::Atoi(*roundTime);
    UWidgetBlueprintLibrary::SetInputMode_GameOnly(UGameplayStatics::GetPlayerController(GetWorld(), 0));
}

void UMenu::PrepareClient(FString trimmedNickname, FString trimmedIp, FString trimmedPort)
{
    USpaceMMAInstance* instance = reinterpret_cast<USpaceMMAInstance*>(GetGameInstance());
    instance->Initialization = USpaceMMAInstance::LevelInitialization::Client;
    instance->ServerIpAddress = trimmedIp;
    instance->ServerPort = FCString::Atoi(*trimmedPort);
    instance->Nickname = trimmedNickname;
    UWidgetBlueprintLibrary::SetInputMode_GameOnly(UGameplayStatics::GetPlayerController(GetWorld(), 0));
}
