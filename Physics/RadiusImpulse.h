#pragma once

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>

/*
	Lua's radiusImpulse, shared by the server and by clients showing the vehicle bricks it breaks flying off, so they agree
	strength is the impulse at the middle, positive pushes away and negative pulls in: something weighing 1, like a player,
	an item, or a broken off brick, gets strength studs a second at the middle, a vehicle weighs one per brick
*/

//How far an impulse reaches per square root of its strength, world units: strength 100 reaches 25 studs
static constexpr float impulseRadiusScale = 2.5f;

//The furthest any impulse reaches, world units
static constexpr float impulseMaxRadius = 200.0f;

/*
	How easily an impulse breaks a destructable vehicle's bricks
	A brick breaks when |strength| * this / (1 + distance squared) is at least its volume in cubic world units,
	with the distance from the middle of the impulse to the nearest part of the brick
	e.g. strength 100 breaks a 2x4 plate (3.2) within about 5 studs, and an 8x4 plate (12.8) within about 2.6
*/
static constexpr float vehicleBrickBreakScale = 1.0f;

inline float impulseRadius(float strength)
{
	return std::min(std::sqrt(std::abs(strength)) * impulseRadiusScale, impulseMaxRadius);
}

//1 at the middle, down to 0 at the edge of its reach
inline float impulseFalloff(float distance, float radius)
{
	return radius <= 0.0f ? 0.0f : std::clamp(1.0f - distance / radius, 0.0f, 1.0f);
}

//Unit direction from the middle of an impulse out to a point, straight up for a point right on it
inline glm::vec3 impulseDirection(const glm::vec3& center, const glm::vec3& point)
{
	glm::vec3 away = point - center;
	float length = glm::length(away);
	return length > 0.001f ? away / length : glm::vec3(0, 1, 0);
}

inline bool impulseBreaksBrick(float strength, float distance, float volume)
{
	return std::abs(strength) * vehicleBrickBreakScale / (1.0f + distance * distance) >= volume;
}
