#pragma once

#include "CoreMinimal.h"
#include <cstdint>
#include "Client/Server/SpaceLog.h"

namespace spacemma
{
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

    namespace packets
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
            S2C_HDamage,
            C2S_HShoot,
            S2C_HShoot,
            B2B_HUpdateVelocity,
            B2B_HRotate,
            S2C_HPlayerMovement,
            B2B_HRopeAttach,
            S2C_HRopeFailed,
            B2B_HRopeDetach,
            B2B_HDeadPlayer,
            B2B_HRespawnPlayer,
            S2C_HInvalidNickname,
            S2C_HInvalidMap,
            S2C_HInvalidData,
            C2S_HInitConnection
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
            uint8_t nicknameLength{};
            uint16_t playerId{};
            NetVector location{};
            NetRotator rotator{};
            std::string nickname{};
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
         * Inform player about getting damage
         */
        struct S2C_Damage
        {
            uint8_t header{ S2C_HDamage };
            uint8_t padding{};
            uint16_t playerId{};
            uint16_t damage{};
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

        /**
         * S2C: inform about a player who was dead
         * C2S: inform about dying
         */
        struct B2B_DeadPlayer final
        {
            uint8_t header{ B2B_HDeadPlayer };
            uint8_t padding{};
            uint16_t playerId{};
        };

        /**
         * S2C: inform about a player who has respawned
         * C2S: inform about respawn
         */
        struct B2B_RespawnPlayer final
        {
            uint8_t header{ B2B_HRespawnPlayer };
            uint8_t padding{};
            uint16_t playerId{};
            NetVector location;
            NetRotator rotator;
        };

        /**
         * Sent before disconnecting client to inform that the reason is an invalid nickname.
         */
        struct S2C_InvalidNickname final
        {
            uint8_t header{ S2C_HInvalidNickname };
        };

        /**
         * Sent before disconnecting client to inform that the reason is an invalid map.
         */
        struct S2C_InvalidMap final
        {
            uint8_t header{ S2C_HInvalidMap };
        };

        /**
         * Sent before disconnecting client to inform that the reason is having provided invalid data.
         */
        struct S2C_InvalidData final
        {
            uint8_t header{ S2C_HInvalidData };
        };

        /**
         * Initialization packet providing player's nickname and map.
         * If the packet or its data is invalid, the server will immediately close connection.
         */
        struct C2S_InitConnection final
        {
            uint8_t header{ C2S_HInitConnection };
            uint8_t mapNameLength{};
            uint8_t nicknameLength{};
            uint8_t padding{};
            std::string mapName{};
            std::string nickname{};
        };

        inline ByteBuffer* createPacketBuffer(gsl::not_null<BufferPool*> bufferPool, S2C_CreatePlayer& packetStruct)
        {
            if (packetStruct.nickname.length() > UINT8_MAX)
            {
                SPACEMMA_ERROR("Unable to create S2C_CreatePlayer packet buffer: nickname too long ({} < {})!",
                               UINT8_MAX, packetStruct.nickname.length());
                return nullptr;
            }
            packetStruct.nicknameLength = packetStruct.nickname.length();
            ByteBuffer* buffer = bufferPool->getBuffer(offsetof(S2C_CreatePlayer, nickname) + packetStruct.nicknameLength);
            memcpy(buffer->getPointer(), &packetStruct, offsetof(S2C_CreatePlayer, nickname));
            memcpy(buffer->getPointer() + offsetof(S2C_CreatePlayer, nickname), packetStruct.nickname.c_str(), packetStruct.nicknameLength);
            return buffer;
        }

        inline ByteBuffer* createPacketBuffer(gsl::not_null<BufferPool*> bufferPool, C2S_InitConnection& packetStruct)
        {
            if (packetStruct.mapName.length() > UINT8_MAX)
            {
                SPACEMMA_ERROR("Unable to create C2S_InitConnection packet buffer: map name too long ({} < {})!",
                               UINT8_MAX, packetStruct.mapName.length());
                return nullptr;
            }
            if (packetStruct.nickname.length() > UINT8_MAX)
            {
                SPACEMMA_ERROR("Unable to create C2S_InitConnection packet buffer: nickname too long ({} < {})!",
                               UINT8_MAX, packetStruct.nickname.length());
                return nullptr;
            }
            packetStruct.mapNameLength = packetStruct.mapName.length();
            packetStruct.nicknameLength = packetStruct.nickname.length();
            ByteBuffer* buffer = bufferPool->getBuffer(offsetof(C2S_InitConnection, mapName) + packetStruct.mapNameLength + packetStruct.nicknameLength);
            memcpy(buffer->getPointer(), &packetStruct, offsetof(C2S_InitConnection, mapName));
            memcpy(buffer->getPointer() + offsetof(C2S_InitConnection, mapName), packetStruct.mapName.c_str(), packetStruct.mapNameLength);
            memcpy(buffer->getPointer() + offsetof(C2S_InitConnection, mapName) + packetStruct.mapNameLength, packetStruct.nickname.c_str(), packetStruct.nicknameLength);
            return buffer;
        }

        inline bool reinterpretPacket(gsl::not_null<ByteBuffer*> packet, S2C_CreatePlayer& packetStruct)
        {
            if (packet->getUsedSize() < offsetof(S2C_CreatePlayer, nickname))
            {
                SPACEMMA_ERROR("Invalid packet {} size ({} > {})!",
                               *packet->getPointer(), offsetof(S2C_CreatePlayer, nickname), packet->getUsedSize());
                return false;
            }
            memcpy(&packetStruct, packet->getPointer(), offsetof(S2C_CreatePlayer, nickname));
            if (packet->getUsedSize() != offsetof(S2C_CreatePlayer, nickname) + packetStruct.nicknameLength)
            {
                SPACEMMA_ERROR("Invalid packet {} size ({} != {})! Expected nickname of {} characters!",
                               *packet->getPointer(), offsetof(S2C_CreatePlayer, nickname) + packetStruct.nicknameLength,
                               packet->getUsedSize(), packetStruct.nicknameLength);
                return false;
            }
            packetStruct.nickname.clear();
            if (packetStruct.nicknameLength > 0)
            {
                packetStruct.nickname.append(
                    reinterpret_cast<char*>(packet->getPointer() + offsetof(S2C_CreatePlayer, nickname)),
                    packetStruct.nicknameLength);
            }
            return true;
        }

        inline bool reinterpretPacket(gsl::not_null<ByteBuffer*> packet, C2S_InitConnection& packetStruct)
        {
            if (packet->getUsedSize() < offsetof(C2S_InitConnection, mapName))
            {
                SPACEMMA_ERROR("Invalid packet {} size ({} > {})!",
                               *packet->getPointer(), offsetof(C2S_InitConnection, mapName), packet->getUsedSize());
                return false;
            }
            memcpy(&packetStruct, packet->getPointer(), offsetof(C2S_InitConnection, mapName));
            if (packet->getUsedSize() != offsetof(C2S_InitConnection, mapName) + packetStruct.mapNameLength + packetStruct.nicknameLength)
            {
                SPACEMMA_ERROR("Invalid packet {} size ({} != {})! Expected map name of {} characters and nickname of {} characters!",
                               *packet->getPointer(), offsetof(C2S_InitConnection, mapName) + packetStruct.mapNameLength + packetStruct.nicknameLength,
                               packet->getUsedSize(), packetStruct.mapNameLength, packetStruct.nicknameLength);
                return false;
            }
            packetStruct.mapName.clear();
            if (packetStruct.mapNameLength > 0)
            {
                packetStruct.mapName.append(
                    reinterpret_cast<char*>(packet->getPointer() + offsetof(C2S_InitConnection, mapName)),
                    packetStruct.mapNameLength);
            }
            packetStruct.nickname.clear();
            if (packetStruct.nicknameLength > 0)
            {
                packetStruct.nickname.append(
                    reinterpret_cast<char*>(packet->getPointer() + offsetof(C2S_InitConnection, mapName) + packetStruct.mapNameLength),
                    packetStruct.nicknameLength);
            }
            return true;
        }

        inline bool reinterpretPacket(gsl::span<uint8_t> buff, size_t& pointerPos, S2C_CreatePlayer& packetStruct)
        {
            if ((buff.size() - pointerPos) < offsetof(S2C_CreatePlayer, nickname))
            {
                SPACEMMA_ERROR("Invalid packet {} size ({} > {}) | Size: {}, Pos: {}!",
                               buff[pointerPos], offsetof(S2C_CreatePlayer, nickname), buff.size() - pointerPos, buff.size(), pointerPos);
                return false;
            }
            memcpy(&packetStruct, buff.data() + pointerPos, offsetof(S2C_CreatePlayer, nickname));
            if (buff.size() < offsetof(S2C_CreatePlayer, nickname) + packetStruct.nicknameLength)
            {
                SPACEMMA_ERROR("Invalid packet {} size ({} > {}) | Size: {}, Pos: {}! Expected nickname of {} characters!",
                               buff[pointerPos], offsetof(S2C_CreatePlayer, nickname) + packetStruct.nicknameLength,
                               buff.size() - pointerPos, buff.size(), pointerPos, packetStruct.nicknameLength);
                return false;
            }
            packetStruct.nickname.clear();
            if (packetStruct.nicknameLength > 0)
            {
                packetStruct.nickname.append(
                    reinterpret_cast<char*>(buff.data() + pointerPos + offsetof(S2C_CreatePlayer, nickname)),
                    packetStruct.nicknameLength);
            }
            pointerPos += offsetof(S2C_CreatePlayer, nickname) + packetStruct.nicknameLength;
            return true;
        }

        inline bool reinterpretPacket(gsl::span<uint8_t> buff, size_t& pointerPos, C2S_InitConnection& packetStruct)
        {
            if ((buff.size() - pointerPos) < offsetof(C2S_InitConnection, mapName))
            {
                SPACEMMA_ERROR("Invalid packet {} size ({} > {}) | Size: {}, Pos: {}!",
                               buff[pointerPos], offsetof(C2S_InitConnection, mapName), buff.size() - pointerPos, buff.size(), pointerPos);
                return false;
            }
            memcpy(&packetStruct, buff.data() + pointerPos, offsetof(C2S_InitConnection, mapName));
            if ((buff.size() - pointerPos) != offsetof(C2S_InitConnection, mapName) + packetStruct.mapNameLength + packetStruct.nicknameLength)
            {
                SPACEMMA_ERROR("Invalid packet {} size ({} != {}) | Size: {}, Pos: {}! Expected map name of {} characters and nickname of {} characters!",
                               buff[pointerPos], offsetof(C2S_InitConnection, mapName) + packetStruct.mapNameLength + packetStruct.nicknameLength,
                               buff.size() - pointerPos, buff.size(), pointerPos, packetStruct.mapNameLength, packetStruct.nicknameLength);
                return false;
            }
            packetStruct.mapName.clear();
            if (packetStruct.mapNameLength > 0)
            {
                packetStruct.mapName.append(
                    reinterpret_cast<char*>(buff.data() + pointerPos + offsetof(C2S_InitConnection, mapName)),
                    packetStruct.mapNameLength);
            }
            packetStruct.nickname.clear();
            if (packetStruct.nicknameLength > 0)
            {
                packetStruct.nickname.append(
                    reinterpret_cast<char*>(buff.data() + pointerPos + offsetof(C2S_InitConnection, mapName) + packetStruct.mapNameLength),
                    packetStruct.nicknameLength);
            }
            pointerPos += offsetof(C2S_InitConnection, mapName) + packetStruct.mapNameLength + packetStruct.nicknameLength;
            return true;
        }

        template<typename T>
        T* reinterpretPacket(gsl::not_null<ByteBuffer*> packet)
        {
            static_assert(!std::is_same<S2C_CreatePlayer, T>());
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
            static_assert(!std::is_same<S2C_CreatePlayer, T>());
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
    }//namespace: packets
}//namespace: spacemma