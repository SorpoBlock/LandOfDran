#include "AcousticProbe.h"

void AcousticProbe::setQuality(int reverbQuality, int occlusionQuality)
{
	static constexpr int rayCounts[3] = { 12, 24, 48 };
	static constexpr float intervalsMS[3] = { 400.0f, 250.0f, 150.0f };
	static constexpr int occlusionRayCounts[3] = { 1, 1, 3 };

	//Off is handled by AudioSystem not asking for measurements or occlusion
	int quality = std::clamp(reverbQuality, 1, 3) - 1;
	measureIntervalMS = intervalsMS[quality];
	occlusionRays = occlusionRayCounts[std::clamp(occlusionQuality, 1, 3) - 1];

	//Spread evenly over a sphere, a Fibonacci spiral from top to bottom
	int count = rayCounts[quality];
	directions.clear();
	for (int a = 0; a < count; a++)
	{
		float y = 1.0f - (a + 0.5f) * 2.0f / count;
		float radius = std::sqrt(std::max(0.0f, 1.0f - y * y));
		float angle = a * 2.39996323f;
		directions.push_back(glm::vec3(std::cos(angle) * radius, y, std::sin(angle) * radius));
	}
}

bool AcousticProbe::measure(float deltaT, const PhysicsWorld& world, const glm::vec3& listener, const btRigidBody* ignore, float& averageDistance, float& enclosure)
{
	sinceMeasureMS += deltaT;
	if (sinceMeasureMS < measureIntervalMS)
		return false;
	sinceMeasureMS = 0;

	auto started = std::chrono::steady_clock::now();

	float distanceSum = 0;
	int levelRays = 0;
	int levelHits = 0;

	for (const glm::vec3& direction : directions)
	{
		//Pointing down hits the ground even in an open field
		if (direction.y < -0.05f)
			continue;

		levelRays++;
		btScalar fraction = world.rayHitFraction(g2b3(listener), g2b3(listener + direction * maxDistance), ignore, nullptr);
		if (fraction < 1)
		{
			levelHits++;
			distanceSum += fraction * maxDistance;
		}
	}

	averageDistance = levelHits > 0 ? distanceSum / levelHits : maxDistance;
	enclosure = levelRays > 0 ? (float)levelHits / levelRays : 0.0f;

	raysThisSecond += levelRays;
	msThisSecond += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();

	return true;
}

float AcousticProbe::occlusion(const PhysicsWorld& world, const glm::vec3& listener, const glm::vec3& source, const btRigidBody* ignoreA, const btRigidBody* ignoreB)
{
	glm::vec3 offset = source - listener;
	float distance = glm::length(offset);
	if (distance < 1.5f)
		return 0.0f;

	auto started = std::chrono::steady_clock::now();

	glm::vec3 forward = offset / distance;
	glm::vec3 side = glm::cross(forward, glm::vec3(0, 1, 0));
	side = glm::length(side) > 0.001f ? glm::normalize(side) : glm::vec3(1, 0, 0);
	glm::vec3 up = glm::cross(side, forward);

	//With more than one ray, the others aim a little to either side of the sound, so a sound just around a corner is partly muffled
	glm::vec3 targets[3] = { source, source + side * 0.75f + up * 0.4f, source - side * 0.75f + up * 0.4f };

	float muffled = 0;
	for (int a = 0; a < occlusionRays; a++)
	{
		//Whatever the sound is coming from inside of, like the brick being planted, doesn't count, see PhysicsWorld::solidThickness
		float thickness = world.solidThickness(g2b3(listener), g2b3(targets[a]), ignoreA, ignoreB);

		//Each stud of solid muffles some of what's left, so thick or layered walls block more than a thin one
		muffled += 1.0f - std::exp(-thickness / muffleThickness);
	}

	//Each ray is cast there and back
	raysThisSecond += occlusionRays * 2;
	msThisSecond += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();

	return muffled / occlusionRays;
}

void AcousticProbe::updateStats(float deltaT)
{
	statsTimerMS += deltaT;
	if (statsTimerMS < 1000.0f)
		return;

	statsTimerMS = 0;
	raysLastSecond = raysThisSecond;
	msLastSecond = msThisSecond;	raysThisSecond = 0;
	msThisSecond = 0;
}

std::string AcousticProbe::getStats() const
{
	char text[64];
	snprintf(text, sizeof(text), "%u rays/s, %.2f ms/s", raysLastSecond, msLastSecond);
	return text;
}

AcousticProbe::AcousticProbe()
{
	setQuality(2, 2);
}
