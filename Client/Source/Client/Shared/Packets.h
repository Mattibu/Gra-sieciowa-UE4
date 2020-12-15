#pragma once

#include "CoreMinimal.h"
#include <cstdint>

namespace spacemma
{
    /// <summary>
    /// Possible values of packet headers.
    /// S - server
    /// C - client
    /// B - both
    /// For each header there should exist a matching struct.
    /// </summary>
    enum Header : uint8_t
    {
        S2C_HCreatePlayer,
        S2C_HDestroyPlayer,
        B2B_HShoot,
        B2B_HChangeSpeed,
        B2B_HRotate,
        S2C_HPlayerMovement,
        B2B_HRopeAttach,
        S2C_HRopeFailed,
        B2B_HRopeDetach
    };

    struct NetVector2D final
    {
        FVector2D asFVector2D() const
        {
            return FVector2D(x, y);
        }
        float x;
        float y;
    };

    struct NetVector final
    {
        FVector asFVector() const
        {
            return FVector(x, y, z);
        }
        float x;
        float y;
        float z;
    };

    struct NetRotator final
    {
        FRotator asFRotator() const
        {
            return FRotator(pitch, yaw, roll);
        }
        float pitch;
        float yaw;
        float roll;
    };

    /**
     * Inform about player spawn
     */
    struct S2C_CreatePlayer
    {
        uint8_t header{ S2C_HCreatePlayer };
        uint8_t padding{};
        uint16_t playerId{};
        NetVector location;
        NetRotator rotator;
    };

    /**
     * Inform about player destruction
     */
    struct S2C_DestroyPlayer
    {
        uint8_t header{ S2C_HDestroyPlayer };
        uint8_t padding{};
        uint16_t playerId{};
    };

    /**
     * S2C: inform players about another player shooting
     * C2S: shooting attempt (might verify the location/rotation, not neccessary)
     */
    struct B2B_Shoot
    {
        uint8_t header{ B2B_HShoot };
        uint8_t padding{};
        uint16_t playerId{};
        NetVector location;
        NetRotator rotator;
    };

    /**
     * S2C: inform players about another player's speed change
     * C2S: speed change attempt (verify the speed)
     */
    struct B2B_ChangeSpeed final
    {
        uint8_t header{ B2B_HChangeSpeed };
        uint8_t padding{};
        uint16_t playerId{};
        float speedValue;
        NetVector speedVector;
    };

    /**
     * S2C: inform players about another player's rotation
     * C2S: rotation attempt (verify the speed)
     */
    struct B2B_Rotate final
    {
        uint8_t header{ B2B_HRotate };
        uint8_t padding{};
        uint16_t playerId{};
        NetVector rotationVector;
    };

    /**
     * Informs about player location, rotation and speed
     */
    struct S2C_PlayerMovement final
    {
        uint8_t header{ S2C_HPlayerMovement };
        uint8_t padding{};
        uint16_t playerId{};
        NetVector location;
        NetRotator rotator;
        float speedValue;
        NetVector speedVector;
    };

    /**
     * S2C: inform about a player who attached a rope
     * C2S: rope attach attempt (may fail)
     */
    struct B2B_RopeAttach final
    {
        uint8_t header{ B2B_HRopeAttach };
        uint8_t padding{};
        uint16_t playerId{};
        NetVector attachPosition;
    };

    /**
     * Failed to attach the rope - either too far or on cooldown
     */
    struct S2C_RopeFailed final
    {
        uint8_t header{ S2C_HRopeFailed };
        uint8_t padding{};
        uint16_t playerId{};
        float ropeCooldown; // let's the client know if the cooldown was incorrect
    };

    /**
     * S2C: inform about a player who detached a rope
     * C2S: rope detach attempt
     */
    struct B2B_RopeDetach final
    {
        uint8_t header{ B2B_HRopeDetach };
        uint8_t padding{};
        uint16_t playerId{};
    };
}