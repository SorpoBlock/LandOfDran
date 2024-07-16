#pragma once

#include "../LandOfDran.h"

/*
	This file defines how we transmit slightly more complex data types 
	like world positions and quaternions over the internet
	That way if I want to go back and make transform data more or less compressed I can easily
*/

//How many bytes addQuaternion will add to the packet
#define QuaternionBytes 4
//How many bytes addPosition will add to the packet
#define PositionBytes 12
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
