#include "ParticleTypes.h"

#include <cmath>

template <typename T>
static void put(std::vector<unsigned char>& bytes, const T& value)
{
	size_t at = bytes.size();
	bytes.resize(at + sizeof(T));
	memcpy(bytes.data() + at, &value, sizeof(T));
}

//One length byte then up to 255 characters
static void putString(std::vector<unsigned char>& bytes, const std::string& text)
{
	size_t length = std::min(text.length(), (size_t)255);
	bytes.push_back((unsigned char)length);
	bytes.insert(bytes.end(), text.begin(), text.begin() + length);
}

template <typename T>
static bool get(const unsigned char* data, size_t length, size_t& at, T& value)
{
	if (at + sizeof(T) > length)
		return false;

	memcpy(&value, data + at, sizeof(T));
	at += sizeof(T);
	return true;
}

static bool getString(const unsigned char* data, size_t length, size_t& at, std::string& text)
{
	if (at + 1 > length)
		return false;

	size_t textLength = data[at];
	at++;
	if (at + textLength > length)
		return false;

	text.assign((const char*)data + at, textLength);
	at += textLength;
	return true;
}

//NaN fails every comparison std::clamp makes, so it has to be replaced first
static float clampFinite(float value, float fallback, float low, float high)
{
	return std::clamp(std::isfinite(value) ? value : fallback, low, high);
}

void ParticleTypeData::clampValues()
{
	for (int a = 0; a < 4; a++)
	{
		for (int channel = 0; channel < 4; channel++)
			colors[a][channel] = clampFinite(colors[a][channel], 1.0f, 0.0f, 1.0f);

		sizes[a] = clampFinite(sizes[a], 1.0f, 0.0f, (float)maxSize);
		times[a] = clampFinite(times[a], 1.0f, a > 0 ? times[a - 1] : 0.0f, 1.0f);
	}

	for (int axis = 0; axis < 3; axis++)
	{
		drag[axis] = clampFinite(drag[axis], 0.0f, 0.0f, 1000.0f);
		gravity[axis] = clampFinite(gravity[axis], 0.0f, -1000.0f, 1000.0f);
	}

	inheritedVelFactor = clampFinite(inheritedVelFactor, 0.0f, -100.0f, 100.0f);
	lifetimeMS = clampFinite(lifetimeMS, 1000.0f, 1.0f, 60000.0f);
	lifetimeVarianceMS = clampFinite(lifetimeVarianceMS, 0.0f, 0.0f, lifetimeMS - 1.0f);
	spinSpeed = clampFinite(spinSpeed, 0.0f, -36000.0f, 36000.0f);
}

void ParticleTypeData::write(std::vector<unsigned char>& bytes) const
{
	putString(bytes, name);
	putString(bytes, texturePath);

	for (int a = 0; a < 4; a++)
	{
		put(bytes, colors[a]);
		put(bytes, sizes[a]);
		put(bytes, times[a]);
	}

	put(bytes, drag);
	put(bytes, gravity);
	put(bytes, inheritedVelFactor);
	put(bytes, lifetimeMS);
	put(bytes, lifetimeVarianceMS);
	put(bytes, spinSpeed);
	bytes.push_back((useInvAlpha ? 1 : 0) | (needsSorting ? 2 : 0) | (lit ? 4 : 0));
}

bool ParticleTypeData::read(const unsigned char* data, size_t length, size_t& at)
{
	if (!getString(data, length, at, name) || !getString(data, length, at, texturePath))
		return false;

	for (int a = 0; a < 4; a++)
	{
		if (!get(data, length, at, colors[a]) || !get(data, length, at, sizes[a]) || !get(data, length, at, times[a]))
			return false;
	}

	unsigned char flags = 0;
	if (!get(data, length, at, drag) || !get(data, length, at, gravity) || !get(data, length, at, inheritedVelFactor) || !get(data, length, at, lifetimeMS)
		|| !get(data, length, at, lifetimeVarianceMS) || !get(data, length, at, spinSpeed) || !get(data, length, at, flags))
		return false;

	useInvAlpha = flags & 1;
	needsSorting = flags & 2;
	lit = flags & 4;
	return true;
}

void ParticleTypeData::getKeys(float lifeFraction, glm::vec4& color, float& size) const
{
	if (lifeFraction <= times[0])
	{
		color = colors[0];
		size = sizes[0];
		return;
	}

	for (int a = 1; a < 4; a++)
	{
		if (lifeFraction > times[a])
			continue;

		float span = times[a] - times[a - 1];
		float blend = span > 0.0001f ? (lifeFraction - times[a - 1]) / span : 1.0f;
		color = glm::mix(colors[a - 1], colors[a], blend);
		size = glm::mix(sizes[a - 1], sizes[a], blend);
		return;
	}

	color = colors[3];
	size = sizes[3];
}

void EmitterTypeData::clampValues()
{
	if (particleTypes.size() > maxParticleTypes)
		particleTypes.resize(maxParticleTypes);

	ejectionPeriodMS = clampFinite(ejectionPeriodMS, 100.0f, 1.0f, 60000.0f);
	//Blockland's rule, otherwise a gap between particles could come out zero or negative
	periodVarianceMS = clampFinite(periodVarianceMS, 0.0f, 0.0f, ejectionPeriodMS - 1.0f);
	ejectionVelocity = clampFinite(ejectionVelocity, 2.0f, 0.0f, 1000.0f);
	velocityVariance = clampFinite(velocityVariance, 0.0f, 0.0f, ejectionVelocity);
	ejectionOffset = clampFinite(ejectionOffset, 0.0f, 0.0f, 1000.0f);
	thetaMin = clampFinite(thetaMin, 0.0f, 0.0f, 180.0f);
	thetaMax = clampFinite(thetaMax, 90.0f, thetaMin, 180.0f);
	phiReferenceVel = clampFinite(phiReferenceVel, 0.0f, -36000.0f, 36000.0f);
	phiVariance = clampFinite(phiVariance, 360.0f, 0.0f, 360.0f);
	lifetimeMS = clampFinite(lifetimeMS, 0.0f, 0.0f, 86400000.0f);
}

void EmitterTypeData::write(std::vector<unsigned char>& bytes) const
{
	putString(bytes, name);
	putString(bytes, uiName);

	size_t count = std::min(particleTypes.size(), (size_t)maxParticleTypes);
	bytes.push_back((unsigned char)count);
	for (size_t a = 0; a < count; a++)
		put(bytes, particleTypes[a]);

	put(bytes, ejectionPeriodMS);
	put(bytes, periodVarianceMS);
	put(bytes, ejectionVelocity);
	put(bytes, velocityVariance);
	put(bytes, ejectionOffset);
	put(bytes, thetaMin);
	put(bytes, thetaMax);
	put(bytes, phiReferenceVel);
	put(bytes, phiVariance);
	put(bytes, lifetimeMS);
}

bool EmitterTypeData::read(const unsigned char* data, size_t length, size_t& at)
{
	unsigned char count = 0;
	if (!getString(data, length, at, name) || !getString(data, length, at, uiName) || !get(data, length, at, count))
		return false;

	particleTypes.resize(count);
	for (unsigned int a = 0; a < count; a++)
	{
		if (!get(data, length, at, particleTypes[a]))
			return false;
	}

	return get(data, length, at, ejectionPeriodMS) && get(data, length, at, periodVarianceMS) && get(data, length, at, ejectionVelocity)
		&& get(data, length, at, velocityVariance) && get(data, length, at, ejectionOffset) && get(data, length, at, thetaMin)
		&& get(data, length, at, thetaMax) && get(data, length, at, phiReferenceVel) && get(data, length, at, phiVariance) && get(data, length, at, lifetimeMS);
}
