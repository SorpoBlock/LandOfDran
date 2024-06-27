#pragma once

#include "../LandOfDran.h"

struct Snapshot
{
	glm::vec3 position = glm::vec3(0, 0, 0);
	glm::quat rotation = glm::quat(1, 0, 0, 0);
	float time = 0;
};

/*
	Takes rotation and position snapshots with associated time from server
	and can be called on at any point to give an interpolated transform for the current time point
*/
class Interpolator
{
	private:

	std::vector<Snapshot> snapshots;

	public:

	void addSnapshot(const glm::vec3& pos, const glm::quat& rot);
	glm::vec3 getPosition();
	glm::quat getRotation();
};