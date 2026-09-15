#pragma once

#include "../LandOfDran.h"
#include <bitset>
#include <cmath>
#include <cstdint>

/*
	This file defines how we transmit slightly more complex data types 
	like world positions and quaternions over the internet
	That way if I want to go back and make transform data more or less compressed I can easily
*/

//How many bytes addQuaternion will add to the packet
#define QuaternionBytes 4
//How many bytes addPosition will add to the packet
#define PositionBytes 6
//How many bytes addVelocity will add to the packet
#define VelocityBytes 6
//How many bytes addAngularVelocity will add to the packet
#define AngularVelocityBytes 4

//TODO: Change these back to inline?

//Copies in a quaternion, possibly after compression, to the next QuaternionBytes bytes of dest
void addQuaternion(enet_uint8* dest, glm::quat quat);

//Copies in a 3 dimensional position, possibly after compression, to the next PositionBytes bytes of dest
void addPosition(enet_uint8 * dest, const glm::vec3& pos);

//Read out the result of addQuaternion
void getQuaternion(enet_uint8 const * src, glm::quat& quat);

//Read out the result of addPosition
void getPosition(enet_uint8 const* src, glm::vec3& pos);

//Read out the result of addPosition
void getVelocity(enet_uint8 const* src, glm::vec3& vel);

//Copies in a 3 dimensional velocity, possibly after compression, to the next VelocityBytes bytes of dest
void addVelocity(enet_uint8* dest, const glm::vec3& vel);

//Read out the result of addPosition
void getAngularVelocity(enet_uint8 const* src, glm::vec3& vel);

//Copies in a 3 dimensional angular velocity, possibly after compression, to the next AngularVelocityBytes bytes of dest
void addAngularVelocity(enet_uint8* dest, const glm::vec3& vel);

//How many bytes addLookDirection will add to the packet
#define LookDirectionBytes 2

//Copies in a unit direction as a yaw and a pitch byte, about 1.4 degrees a step, only meant for turning heads
inline void addLookDirection(enet_uint8* dest, const glm::vec3& dir)
{
	const float pi = glm::pi<float>();
	float yaw = std::atan2(dir.x, dir.z);
	float pitch = std::asin(std::clamp(dir.y, -1.0f, 1.0f));
	dest[0] = (enet_uint8)((int)std::lround((yaw + pi) / (2.0f * pi) * 256.0f) & 255);
	dest[1] = (enet_uint8)std::clamp((int)std::lround((pitch / pi + 0.5f) * 255.0f), 0, 255);
}

//Read out the result of addLookDirection
inline void getLookDirection(enet_uint8 const* src, glm::vec3& dir)
{
	const float pi = glm::pi<float>();
	float yaw = src[0] / 256.0f * 2.0f * pi - pi;
	float pitch = (src[1] / 255.0f - 0.5f) * pi;
	dir = glm::vec3(std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch));
}
