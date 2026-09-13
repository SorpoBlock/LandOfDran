#pragma once

#include "../Physics/PhysicsWorld.h"

/*
	Client only: raycasts for environmental audio
	measure looks around the listener to judge how big and closed in the space is, for AudioSystem::setListenerSpace
	occlusion checks whether anything is between the listener and a sound, for AudioSystem's OcclusionTest
*/
class AcousticProbe
{
	std::vector<glm::vec3> directions;
	float measureIntervalMS = 250.0f;
	float sinceMeasureMS = 0.0f;
	int occlusionRays = 1;

	//Totals for getStats, refreshed once a second by updateStats
	unsigned int raysThisSecond = 0;
	double msThisSecond = 0;
	float statsTimerMS = 0;
	unsigned int raysLastSecond = 0;
	double msLastSecond = 0;

public:

	//Rays stop this far out, farther surfaces count as open space
	static constexpr float maxDistance = 40.0f;

	//audio/reverbquality and audio/occlusionquality: 0 off, 1-3 low to high. Sets how many rays each uses and how often measure casts
	void setQuality(int reverbQuality, int occlusionQuality);

	/*
		Casts the probe rays once every measure interval, returns true when it did and filled in the results
		averageDistance: to the level and upward surfaces hit, maxDistance if none
		enclosure: fraction of level and upward rays that hit something, 0 out in the open, 1 closed in on every side
		The ground doesn't count toward either, it's under you in the open too
	*/
	bool measure(float deltaT, const PhysicsWorld& world, const glm::vec3& listener, const btRigidBody* ignore, float& averageDistance, float& enclosure);

	//0 for a clear path from listener to source through 1 for completely blocked, skipping the two bodies given
	float occlusion(const PhysicsWorld& world, const glm::vec3& listener, const glm::vec3& source, const btRigidBody* ignoreA, const btRigidBody* ignoreB);

	//Call every frame
	void updateStats(float deltaT);

	//Rays and milliseconds spent over the last second, for the debug menu
	std::string getStats() const;

	AcousticProbe();
};
