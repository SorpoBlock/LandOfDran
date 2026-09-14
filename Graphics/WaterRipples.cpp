#include "WaterRipples.h"

//World units per second the leading ring spreads out at
static constexpr float spreadSpeed = 3.0f;

//Fainter than this, a ripple is gone
static constexpr float faintest = 0.03f;
static constexpr float maxAge = 8.0f;

//water.frag has flattened the surface out by 150 units from the camera anyway
static constexpr float drawDistance = 160.0f;

//When a lot is going on at once, the faintest ripple makes way for a new one
static constexpr unsigned int maxKept = 64;

//Height of the strongest ripple from something with a size of 1, and a cap on how steep one can make the surface
static constexpr float heightScale = 0.07f;
static constexpr float maxSlope = 0.6f;

float WaterRipples::Ripple::fade() const
{
	//Dies down over time, and spreads the same disturbance along a longer ring as it grows
	return strength * std::exp(-decay * age) / std::sqrt(1.0f + spreadSpeed * age / (size + 1.0f));
}

float WaterRipples::Ripple::frontRadius() const
{
	return size + spreadSpeed * age;
}

void WaterRipples::add(const glm::vec2& center, float strength, float size, float decay)
{
	Ripple ripple;
	ripple.center = center;
	ripple.strength = std::clamp(strength, 0.0f, 1.0f);
	ripple.size = std::clamp(size, 0.3f, 8.0f);
	ripple.decay = decay;

	if (ripples.size() < maxKept)
	{
		ripples.push_back(ripple);
		return;
	}

	auto weakest = std::min_element(ripples.begin(), ripples.end(), [](const Ripple& a, const Ripple& b) { return a.fade() < b.fade(); });
	if (weakest->fade() < ripple.fade())
		*weakest = ripple;
}

void WaterRipples::update(float deltaSeconds)
{
	for (Ripple& ripple : ripples)
		ripple.age += deltaSeconds;

	std::erase_if(ripples, [](const Ripple& ripple) { return ripple.age > maxAge || ripple.fade() < faintest; });
}

void WaterRipples::upload(const Program* waterShader, const glm::vec3& cameraPosition) const
{
	struct Candidate
	{
		const Ripple* ripple;
		float weight;
	};

	Candidate candidates[maxKept];
	int candidateCount = 0;

	glm::vec2 camera(cameraPosition.x, cameraPosition.z);
	for (const Ripple& ripple : ripples)
	{
		float outsideRing = glm::length(camera - ripple.center) - ripple.frontRadius();
		if (outsideRing > drawDistance)
			continue;

		//Strong ripples near the camera first
		candidates[candidateCount++] = { &ripple, ripple.fade() / (1.0f + std::max(0.0f, outsideRing) / 30.0f) };
	}

	int drawn = std::min(candidateCount, maxDrawn);
	std::partial_sort(candidates, candidates + drawn, candidates + candidateCount, [](const Candidate& a, const Candidate& b) { return a.weight > b.weight; });

	//Per ripple, rings: center x and z, leading ring radius, width of the band of rings trailing it; waves: steepness, wave number
	float rings[maxDrawn * 4];
	float waves[maxDrawn * 2];
	for (int a = 0; a < drawn; a++)
	{
		const Ripple& ripple = *candidates[a].ripple;

		//Rings from bigger things are further apart, and they spread apart a bit as they go
		float wavelength = (0.6f + 0.25f * ripple.size) * (1.0f + 0.15f * ripple.age);
		float waveNumber = 6.2831853f / wavelength;
		float frontRadius = ripple.frontRadius();

		rings[a * 4] = ripple.center.x;
		rings[a * 4 + 1] = ripple.center.y;
		rings[a * 4 + 2] = frontRadius;
		//The band never reaches past the center, where the rings would pinch together
		rings[a * 4 + 3] = std::min(wavelength * (2.0f + 0.8f * ripple.age), frontRadius);

		float height = heightScale * std::sqrt(ripple.size) * ripple.fade();
		waves[a * 2] = std::min(height * waveNumber, maxSlope);
		waves[a * 2 + 1] = waveNumber;
	}

	glUniform1i(waterShader->getUniformLocation("RippleCount"), drawn);
	if (drawn > 0)
	{
		glUniform4fv(waterShader->getUniformLocation("RippleRings"), drawn, rings);
		glUniform2fv(waterShader->getUniformLocation("RippleWaves"), drawn, waves);
	}
}
