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
        S2C_HProvidePlayerId,
        S2C_HCreatePlayer,
        S2C_HDestroyPlayer,
        C2S_HShoot,
        S2C_HShoot,
        B2B_HUpdateVelocity,
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
        NetVector2D() = default;
        NetVector2D(FVector2D fvec) : x(fvec.X), y(fvec.Y) {}
        float x;
        float y;
    };

    struct NetVector final
    {
        FVector asFVector() const
        {
            return FVector(x, y, z);
        }
        NetVector() = default;
        NetVector(FVector fvec) : x(fvec.X), y(fvec.Y), z(fvec.Z) {}
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
        NetRotator() = default;
        NetRotator(FRotator frot) : pitch(frot.Pitch), yaw(frot.Yaw), roll(frot.Roll) {}
        float pitch;
        float yaw;
        float roll;
    };

    /**
     * Sent to every player after creating a connection.
     * Tells the player what his/her ID is.
     */
    struct S2C_ProvidePlayerId
    {
        uint8_t header{ S2C_HProvidePlayerId };
        uint8_t padding{};
        uint16_t playerId{};
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
     * inform players about another player shooting
     */
    struct S2C_Shoot
    {
        uint8_t header{ S2C_HShoot };
        uint8_t padding{};
        uint16_t playerId{};
        uint16_t distance{};
        NetVector location;
        NetRotator rotator;
    };

    /**
     * shooting attempt (might verify the location/rotation, not neccessary)
     */
    struct C2S_Shoot
    {
        uint8_t header{ C2S_HShoot };
        uint8_t padding{};
        uint16_t playerId{};
        NetVector location;
        NetRotator rotator;
    };

    /**
     * S2C: inform players about another player's velocity update
     * C2S: velocity change attempt (verify the velocity)
     */
    struct B2B_UpdateVelocity final
    {
        uint8_t header{ B2B_HUpdateVelocity };
        uint8_t padding{};
        uint16_t playerId{};
        NetVector velocity;
    };

    /**
     * S2C: inform players about another player's rotation
     * C2S: rotation attempt
     */
    struct B2B_Rotate final
    {
        uint8_t header{ B2B_HRotate };
        uint8_t padding{};
        uint16_t playerId{};
        NetRotator rotator;
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
        NetVector velocity;
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

    template<typename T>
    T* reinterpretPacket(gsl::not_null<ByteBuffer*> packet)
    {
        if (packet->getUsedSize() != sizeof(T))
        {
            SPACEMMA_ERROR("Invalid packet {} size ({} != {})!",
                           *packet->getPointer(), sizeof(T), packet->getUsedSize());
            return nullptr;
        }
        return reinterpret_cast<T*>(packet->getPointer());
    }

    template<typename T>
    T* reinterpretPacket(gsl::span<uint8_t> buff, size_t& pointerPos)
    {
        if ((buff.size() - pointerPos) < sizeof(T))
        {
            SPACEMMA_ERROR("Invalid packet {} size ({} != {}) | Size: {}, Pos: {}!",
                           buff.data(), sizeof(T), buff.size() - pointerPos, buff.size(), pointerPos);
            return nullptr;
        }
        size_t currPos = pointerPos;
        pointerPos += sizeof(T);
        return reinterpret_cast<T*>(buff.data() + currPos);
    }
}