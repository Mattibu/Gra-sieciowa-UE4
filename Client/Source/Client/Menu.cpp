#include "Menu.h"

#include <WS2tcpip.h>

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

void UMenu::StartServer(FString trimmedIp, FString trimmedPort, FString map)
{

}

void UMenu::StartClient(FString trimmedNickname, FString trimmedIp, FString trimmedPort, FString map)
{

}
