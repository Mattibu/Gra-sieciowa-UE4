// Fill out your copyright notice in the Description page of Project Settings.


#include "ConnectionManager.h"

// Sets default values
AConnectionManager::AConnectionManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AConnectionManager::BeginPlay()
{
	Super::BeginPlay();
	//std::string a = std::string(*IpAddress);
	IpAddress = "127.0.0.1";
	Port = 4444;
	//createServer(IpAddress, Port);
	//FPlatformProcess::Sleep(10);
	connectToServer(IpAddress, Port);
}

// Called every frame
void AConnectionManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	receiveData();

}
void AConnectionManager::connectToServer(FString ipAddress, int32 socketNumber)
{/*
	DebugLogger::logInfo(FString::Printf(TEXT("Try to connect with server %s:%i"), *ipAddress, socketNumber));
	FIPv4Address serverAddress;
	if(!FIPv4Address::Parse(ipAddress, serverAddress))
		DebugLogger::logError("Error during converting IP adress");
	else
		DebugLogger::logInfo("IP address converted");
	
	TSharedRef<FInternetAddr> addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	//addr->SetIp(serverAddress.Value);
	//addr->SetPort(socketNumber);

	addr->SetIp(serverAddress.Value);
	addr->SetPort(socketNumber);
	
	uint32 address;
	addr->GetIp(address);
	DebugLogger::logInfo(FString::Printf(TEXT("IP address: %u, port %i"), serverAddress.Value, socketNumber));

	FString clientIpAddress = "127.0.0.1";
	uint16 clientSocket = 50001;

	FIPv4Address clientAddress;
	FIPv4Address::Parse(ipAddress, clientAddress);
	FIPv4Endpoint clientEndpoint(serverAddress, clientSocket);
	socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("default"), false);
	socket = FTcpSocketBuilder(TEXT("Tcp socket")).AsBlocking().AsReusable();
	ESocketConnectionState connectionState = socket->GetConnectionState();
	if(connectionState == ESocketConnectionState::SCS_Connected)
		DebugLogger::logInfo("STATE: Connected");
	else if (connectionState == ESocketConnectionState::SCS_ConnectionError)
			DebugLogger::logError("STATE: Connection ERROR");
	else if (connectionState == ESocketConnectionState::SCS_NotConnected)
		DebugLogger::logWarning("STATE: NOT connected");

	if (socket->Connect(*addr))
		DebugLogger::logInfo("Connected with socket");
	else
		DebugLogger::logError("Unable to connect server");
	connectionState = socket->GetConnectionState();
	if (connectionState == ESocketConnectionState::SCS_Connected)
		DebugLogger::logInfo("STATE: Connected");
	else if (connectionState == ESocketConnectionState::SCS_ConnectionError)
		DebugLogger::logError("STATE: Connection ERROR");
	else if (connectionState == ESocketConnectionState::SCS_NotConnected)
		DebugLogger::logWarning("STATE: NOT connected");

	
	

	
	int32 sent = 0;
	const uint8 data = 1;
	socket->Send(&data, sizeof(data), sent);
	DebugLogger::logInfo(FString::Printf(TEXT("Sent %i bytes of data"), sent));
    */

	

	socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, ipAddress, false);
	if (!socket)
		return;

	ESocketType socketType =  socket->GetSocketType();
	if(socketType == SOCKTYPE_Unknown)
		DebugLogger::logInfo("Type: SOCKTYPE_Unknown");
	if (socketType == SOCKTYPE_Datagram)
		DebugLogger::logInfo("Type: SOCKTYPE_Datagram");
	if (socketType == SOCKTYPE_Streaming)
		DebugLogger::logInfo("Type: SOCKTYPE_Streaming");

	FIPv4Address ip;
	FIPv4Address::Parse(ipAddress, ip);

	TSharedPtr<FInternetAddr> InternetAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	InternetAddress->SetAnyAddress();
	InternetAddress->SetIp(ip.Value);
	InternetAddress->SetPort(socketNumber);
	ESocketConnectionState connectionState = socket->GetConnectionState();
	if (connectionState == ESocketConnectionState::SCS_Connected)
		DebugLogger::logInfo("STATE: Connected");
	else if (connectionState == ESocketConnectionState::SCS_ConnectionError)
		DebugLogger::logError("STATE: Connection ERROR");
	else if (connectionState == ESocketConnectionState::SCS_NotConnected)
		DebugLogger::logWarning("STATE: NOT connected");

	if (socket->Connect(*InternetAddress))
		DebugLogger::logInfo("Connected with socket");
	else
		DebugLogger::logError("Unable to connect server");

	ESocketErrors err = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode();
	DebugLogger::logInfo(FString::Printf(TEXT("ERROR: %s"), ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetSocketError(err)));

	connectionState = socket->GetConnectionState();
	if (connectionState == ESocketConnectionState::SCS_Connected)
		DebugLogger::logInfo("STATE: Connected");
	else if (connectionState == ESocketConnectionState::SCS_ConnectionError)
		DebugLogger::logError("STATE: Connection ERROR");
	else if (connectionState == ESocketConnectionState::SCS_NotConnected)
		DebugLogger::logWarning("STATE: NOT connected");

}

uint32_t litendtoint(uint8_t* lit_int) {
	return (uint32_t)lit_int[0] << 0
		| (uint32_t)lit_int[1] << 8
		| (uint32_t)lit_int[2] << 16
		| (uint32_t)lit_int[3] << 24;
}

void AConnectionManager::receiveData()
{
	uint32 a;
	if (socket->HasPendingData(a))
	{
		int32 readSize;
		uint8* data = new uint8[8000];
		socket->Recv(data, 8000, readSize);
		DebugLogger::logInfo(FString::Printf(TEXT("Received %i bytes of data"), readSize));
		while (readSize > 0)
		{
			uint8_t myPlayerId = data[0];
			uint8_t otherPlayerId = data[1];

			float tmp;
			memcpy(&tmp, data + 2, sizeof(tmp));
			enemyPositions[otherPlayerId].X = tmp;
			memcpy(&tmp, data + 2 + sizeof(float), sizeof(tmp));
			enemyPositions[otherPlayerId].Y = tmp;
			memcpy(&tmp, data + 2 + (2 * sizeof(int)), sizeof(tmp));
			enemyPositions[otherPlayerId].Z = tmp;
			DebugLogger::logInfo(FString::Printf(TEXT("%d: Received data player %d: %F, %F, %F"), myPlayerId, otherPlayerId, enemyPositions[otherPlayerId].X, enemyPositions[otherPlayerId].Y, enemyPositions[otherPlayerId].Z));
			readSize -= 40;//TODO:tmp
		}
		/*for (int i = 0; i < readSize; i++)
		{
			DebugLogger::logInfo(FString::Printf(TEXT("Received: %u"), data[i]));
		}*/
		delete data;
	}

}

FVector AConnectionManager::getPosition(int id)
{
	return enemyPositions[id];
}


void AConnectionManager::createServer(FString ipAddress, int32 socketNumber)
{
	FIPv4Address IPAddress;
	FIPv4Address::Parse(ipAddress, IPAddress);
	FIPv4Endpoint Endpoint(IPAddress, (uint16)socketNumber);

	ListenSocket = FTcpSocketBuilder(TEXT("TcpSocket")).AsBlocking();
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if(ListenSocket->Bind(*SocketSubsystem->CreateInternetAddr(Endpoint.Address.Value, Endpoint.Port)))
		DebugLogger::logInfo("Bind succesfully");
	else
		DebugLogger::logError("Bind ERROR ");

	ListenSocket2 = FTcpSocketBuilder(TEXT("TcpSocket")).AsReusable();
	ISocketSubsystem* SocketSubsystem2 = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (ListenSocket2->Bind(*SocketSubsystem2->CreateInternetAddr(Endpoint.Address.Value, Endpoint.Port)))
		DebugLogger::logInfo("Bind succesfully");
	else
		DebugLogger::logError("Bind ERROR ");

	if (ListenSocket->Listen(8))
		DebugLogger::logInfo("Listen succesfully");
	else
		DebugLogger::logError("Listen ERROR ");
	
	if (ListenSocket2->Listen(8))
		DebugLogger::logInfo("Listen succesfully");
	else
		DebugLogger::logError("Listen ERROR ");

	ESocketType socketType = ListenSocket->GetSocketType();
	if (socketType == SOCKTYPE_Unknown)
		DebugLogger::logInfo("Type: SOCKTYPE_Unknown");
	if (socketType == SOCKTYPE_Datagram)
		DebugLogger::logInfo("Type: SOCKTYPE_Datagram");
	if (socketType == SOCKTYPE_Streaming)
		DebugLogger::logInfo("Type: SOCKTYPE_Streaming");

}