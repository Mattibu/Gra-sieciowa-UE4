#include "BasicDoor.h"

ABasicDoor::ABasicDoor()
{
    PrimaryActorTick.bCanEverTick = true;

}

void ABasicDoor::BeginPlay()
{
    Super::BeginPlay();
    closedLocation = GetActorLocation();
}

void ABasicDoor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    bool updatePos = false;
    if (isOpen && openProgress != 1.0f)
    {
        updatePos = true;
        openProgress += DeltaTime / SlideTime;
        if (openProgress > 1.0f)
        {
            openProgress = 1.0f;
        }
    } else if (!isOpen && openProgress != 0.0f)
    {
        updatePos = true;
        openProgress -= DeltaTime / SlideTime;
        if (openProgress < 0.0f)
        {
            openProgress = 0.0f;
        }
    }
    if (updatePos)
    {
        SetActorLocation(closedLocation + OpenOffset * openProgress);
    }
}

bool ABasicDoor::IsOpen()
{
    return isOpen;
}

float ABasicDoor::GetOpenProgress()
{
    return openProgress;
}

void ABasicDoor::OpenDoor()
{
    isOpen = true;
}

void ABasicDoor::CloseDoor()
{
    isOpen = false;
}

void ABasicDoor::OpenInstantly()
{
    isOpen = true;
    openProgress = 1.0f;
    SetActorLocation(closedLocation + OpenOffset);
}

void ABasicDoor::CloseInstantly()
{
    isOpen = false;
    openProgress = 0.0f;
    SetActorLocation(closedLocation);
}

void ABasicDoor::SetOpenInstantly(bool open)
{
    if (open)
    {
        OpenInstantly();
    } else
    {
        CloseInstantly();
    }
}

void ABasicDoor::SetOpen(bool open)
{
    if (open)
    {
        OpenDoor();
    } else
    {
        CloseDoor();
    }
}

void ABasicDoor::SetState(bool open, bool instantly)
{
    if (open)
    {
        if (instantly)
        {
            OpenInstantly();
        } else
        {
            OpenDoor();
        }
    } else
    {
        if (instantly)
        {
            CloseInstantly();
        } else
        {
            CloseDoor();
        }
    }
}

