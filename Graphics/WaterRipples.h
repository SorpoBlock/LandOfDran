#pragma once

#include "../LandOfDran.h"
#include "Program.h"

/*
	Client only: rings spreading across the water from things that went into, came out of, or moved along it
	Purely visual and worked out by each client on its own, see LoopClient::makeWaterRipples for what starts one
	They only bend the water's normals in water.frag, the surface mesh is far too coarse to actually ripple
*/
class WaterRipples
{
	struct Ripple
	{
		//Where on the surface it started, x and z
		glm::vec2 center = glm::vec2(0);
		//Seconds
		float age = 0;
		//0 to 1
		float strength = 0;
		//Roughly the radius of what made it
		float size = 1;
		//Per second, how quickly it dies down
		float decay = 0.5f;

		//How strong it still is after dying down and spreading out
		float fade() const;
		//Radius of the leading ring
		float frontRadius() const;
	};

	std::vector<Ripple> ripples;

	public:

	//Must match MAX_RIPPLES in water.frag, the ones nearest the camera are drawn when there are more
	static constexpr int maxDrawn = 16;

	//Ripples from splashes last a few seconds, from wading or swimming decay is higher so a wake doesn't pile up
	void add(const glm::vec2& center, float strength, float size, float decay = 0.5f);

	//Ages every ripple and forgets faded ones
	void update(float deltaSeconds);

	//Sets the ripple uniforms on the already bound water shader
	void upload(const Program* waterShader, const glm::vec3& cameraPosition) const;

	void clear() { ripples.clear(); }
};
