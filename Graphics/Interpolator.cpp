#include "Interpolator.h"

void Interpolator::addSnapshot(const glm::vec3& pos, const glm::quat& rot)
{
	//Discard all obsolete snapshots before the current interpolation start point
	auto iter = snapshots.begin();
	while (iter != snapshots.end())
	{
		if ((*iter).time < SDL_GetTicks() && iter != snapshots.begin())
			iter = snapshots.erase(iter);
		else
			++iter;
	}

	//This should allow a buffer of one snapshot or so, assuming good internet
	float time = SDL_GetTicks() + 40.0f;
	if (snapshots.size() > 0)
		time = snapshots.back().time + 40.0f;

	snapshots.emplace_back(Snapshot({ pos,rot,time }));
}

glm::vec3 Interpolator::getPosition()
{
	float curTime = SDL_GetTicks();

	for (unsigned int a = 0; a < snapshots.size(); a++)
	{
		if (snapshots[a].time < curTime)
		{
			if (a + 1 >= snapshots.size())
				return snapshots[a].position;

			if (snapshots[a + 1].time < curTime)
				continue;

			float timeBetween = snapshots[a + 1].time - snapshots[a].time;
			float timePast = curTime - snapshots[a].time;
			float progress = timePast / timeBetween;

			return lerp(snapshots[a].position, snapshots[a + 1].position, progress); 
		}
	}

	return glm::vec3(0, 0, 0);
}

glm::quat Interpolator::getRotation()
{
	float curTime = SDL_GetTicks();

	for (unsigned int a = 0; a < snapshots.size(); a++)
	{
		if (snapshots[a].time < curTime)
		{
			if (a + 1 >= snapshots.size())
				return snapshots[a].position;

			if (snapshots[a + 1].time < curTime)
				continue;

			float timeBetween = snapshots[a + 1].time - snapshots[a].time;
			float timePast = curTime - snapshots[a].time;
			float progress = timePast / timeBetween;

			return glm::slerp(snapshots[a].rotation, snapshots[a + 1].rotation, progress);
		}
	}

	return glm::quat(1, 0, 0, 0);
}

