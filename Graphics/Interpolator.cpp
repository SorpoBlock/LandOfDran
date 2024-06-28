#include "Interpolator.h"

//This is how many milliseconds are *meant* to be between snapshots from the server
//Lag or packet loss or low frame rate can cause this to be higher or lower
float nominalSnapshotTime = 25.0f;

void Interpolator::addSnapshot(const glm::vec3& pos, const glm::quat& rot,float idealBufferSize)
{
	//Discard all obsolete snapshots before the current interpolation start point
	for (int i = snapshots.size() - 1; i >= 0; i--)
	{
		if (snapshots[i].time < getTicksMS())
		{
			//Keep only one snapshot before the current time
			if (i != 0)
				snapshots.erase(snapshots.begin(), snapshots.begin() + (i-1));
			break;
		}
	}

	float lastFrameTime = getTicksMS();
	if (snapshots.size() > 0)
		lastFrameTime = snapshots.back().time;

	//If we're running low on snapshots (server lag?) then interpolate slower, and vice-versa
	float timeBetween = nominalSnapshotTime;
	float diff = snapshots.size() - idealBufferSize;
	diff = 1.0 - (diff * 0.1);
	if (diff < 1.0)
		diff = sqrt(diff);
	timeBetween *= diff;

	float time = lastFrameTime + timeBetween;

	snapshots.emplace_back(Snapshot({ pos,rot,time }));
}

glm::vec3 Interpolator::getPosition()
{
	float curTime = getTicksMS();

	for (unsigned int a = 0; a < snapshots.size(); a++)
	{
		if (snapshots[a].time < curTime)
		{
			if (a + 1 >= snapshots.size())
				return snapshots[a].position;

			if (snapshots[a + 1].time < curTime)
				continue;

			//std::cout << "Using " << a << " of " << snapshots.size() << "\n";

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
	float curTime = getTicksMS();

	for (unsigned int a = 0; a < snapshots.size(); a++)
	{
		if (snapshots[a].time < curTime)
		{
			if (a + 1 >= snapshots.size())
				return snapshots[a].rotation;

			if (snapshots[a + 1].time < curTime)
				continue;

			float timeBetween = snapshots[a + 1].time - snapshots[a].time;
			float timePast = curTime - snapshots[a].time;
			float progress = timePast / timeBetween;

			return glm::slerp(snapshots[a].rotation, snapshots[a + 1].rotation, progress);
		}
	}

	//std::cout<<"No snapshots to interpolate between\n";
	return glm::quat(1, 0, 0, 0);
}

