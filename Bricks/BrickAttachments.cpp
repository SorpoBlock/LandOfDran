#include "BrickAttachments.h"

#include "Brick.h"
#include "../SimObjects/Light.h"

#include <cmath>

static float finiteOr(float value, float fallback)
{
	return std::isfinite(value) ? value : fallback;
}

unsigned char BrickAttachments::getFlags() const
{
	return (musicName.empty() ? 0 : BrickAttachment_Music) | (hasLight ? BrickAttachment_Light : 0) | (emitterName.empty() ? 0 : BrickAttachment_Emitter);
}

void BrickAttachments::clampValues()
{
	musicName = musicName.substr(0, maxNameLength);
	emitterName = emitterName.substr(0, maxNameLength);

	//Same ranges as Lua's startSoundLoop
	musicVolume = std::clamp(finiteOr(musicVolume, 1.0f), 0.0f, 1.0f);
	musicPitch = std::clamp(finiteOr(musicPitch, 1.0f), 0.05f, 10.0f);

	for (int axis = 0; axis < 3; axis++)
	{
		lightColor[axis] = std::clamp(finiteOr(lightColor[axis], 1.0f), 0.0f, 1.0f);
		lightDirection[axis] = finiteOr(lightDirection[axis], 0.0f);
		lightOffset[axis] = std::clamp(finiteOr(lightOffset[axis], 0.0f), -maxLightOffset, maxLightOffset);
	}

	lightBrightness = std::clamp(finiteOr(lightBrightness, 0.0f), 0.0f, Light::maxBrightness);
	lightFlicker = std::clamp(finiteOr(lightFlicker, 0.0f), 0.0f, Light::maxFlicker);
	lightCoronaWidth = std::clamp(finiteOr(lightCoronaWidth, 0.0f), 0.0f, Light::maxCoronaWidth);
	lightSpin = std::clamp(finiteOr(lightSpin, 0.0f), -Light::maxSpin, Light::maxSpin);

	lightConeAngle = finiteOr(lightConeAngle, 0.0f);
	lightConeAngle = lightConeAngle <= 0.0f ? 0.0f : std::clamp(lightConeAngle, 1.0f, Light::maxConeAngle);

	float length = glm::length(lightDirection);
	lightDirection = length > 0.0001f ? lightDirection / length : glm::vec3(0, -1, 0);
}

void BrickAttachments::resetLight()
{
	const BrickAttachments defaults;
	lightColor = defaults.lightColor;
	lightBrightness = defaults.lightBrightness;
	lightFlicker = defaults.lightFlicker;
	lightCoronaWidth = defaults.lightCoronaWidth;
	lightConeAngle = defaults.lightConeAngle;
	lightDirection = defaults.lightDirection;
	lightSpin = defaults.lightSpin;
	lightOffset = defaults.lightOffset;
}

void BrickAttachments::writeParts(const std::function<void(const void*, size_t)>& writeBytes) const
{
	auto writeName = [&](const std::string& name)
	{
		unsigned char length = (unsigned char)std::min(name.length(), maxNameLength);
		writeBytes(&length, 1);
		writeBytes(name.data(), length);
	};

	if (!musicName.empty())
	{
		writeName(musicName);
		writeBytes(&musicVolume, sizeof(float));
		writeBytes(&musicPitch, sizeof(float));
	}

	if (hasLight)
	{
		const float light[15] = { lightColor.r, lightColor.g, lightColor.b, lightBrightness, lightFlicker, lightCoronaWidth, lightConeAngle,
			lightDirection.x, lightDirection.y, lightDirection.z, lightSpin, lightOffset.x, lightOffset.y, lightOffset.z };
		writeBytes(light, sizeof(light));
	}

	if (!emitterName.empty())
		writeName(emitterName);
}

bool BrickAttachments::readParts(unsigned char flags, const std::function<bool(void*, size_t)>& readBytes)
{
	auto readName = [&](std::string& name) -> bool
	{
		unsigned char length;
		if (!readBytes(&length, 1))
			return false;

		name.resize(length);
		return length == 0 || readBytes(name.data(), length);
	};

	musicName = "";
	emitterName = "";
	hasLight = flags & BrickAttachment_Light;

	if ((flags & BrickAttachment_Music) && (!readName(musicName) || !readBytes(&musicVolume, sizeof(float)) || !readBytes(&musicPitch, sizeof(float))))
		return false;

	if (hasLight)
	{
		float light[15];
		if (!readBytes(light, sizeof(light)))
			return false;

		lightColor = glm::vec3(light[0], light[1], light[2]);
		lightBrightness = light[3];
		lightFlicker = light[4];
		lightCoronaWidth = light[5];
		lightConeAngle = light[6];
		lightDirection = glm::vec3(light[7], light[8], light[9]);
		lightSpin = light[10];
		lightOffset = glm::vec3(light[11], light[12], light[13]);
	}

	if ((flags & BrickAttachment_Emitter) && !readName(emitterName))
		return false;

	return true;
}

void BrickAttachments::write(std::vector<unsigned char>& bytes) const
{
	bytes.push_back(getFlags());
	writeParts([&bytes](const void* data, size_t count)
	{
		bytes.insert(bytes.end(), (const unsigned char*)data, (const unsigned char*)data + count);
	});
}

bool BrickAttachments::read(const unsigned char* data, size_t length, size_t& at)
{
	if (at >= length)
		return false;

	unsigned char flags = data[at++];
	return readParts(flags, [&](void* out, size_t count)
	{
		if (at + count > length)
			return false;

		memcpy(out, data + at, count);
		at += count;
		return true;
	});
}
