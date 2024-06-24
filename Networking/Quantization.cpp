#include "Quantization.h"

void addQuaternion(enet_uint8* dest, const glm::quat& quat)
{
	//TODO: Compression. Quaternions can be 'reasonably' transmitted in 30 bits rather than 128bits as seen here
	memcpy(dest + sizeof(float) * 0, &quat.w, sizeof(float));
	memcpy(dest + sizeof(float) * 1, &quat.x, sizeof(float));
	memcpy(dest + sizeof(float) * 2, &quat.y, sizeof(float));
	memcpy(dest + sizeof(float) * 3, &quat.z, sizeof(float));
}

void addPosition(enet_uint8* dest, const glm::vec3& pos)
{
	//TODO: Compression? Positions could be plausabily bounded between -/+2048 with 1/32s precision for 16 bits per component instead of 32
	//Bit less of a good trade than the quat compression though...
	memcpy(dest + sizeof(float) * 0, &pos.x, sizeof(float));
	memcpy(dest + sizeof(float) * 1, &pos.y, sizeof(float));
	memcpy(dest + sizeof(float) * 2, &pos.z, sizeof(float));
}

void getQuaternion(enet_uint8 const* src, glm::quat& quat)
{
	memcpy(&quat.w, src + sizeof(float) * 0, sizeof(float));
	memcpy(&quat.x, src + sizeof(float) * 1, sizeof(float));
	memcpy(&quat.y, src + sizeof(float) * 2, sizeof(float));
	memcpy(&quat.z, src + sizeof(float) * 3, sizeof(float));
}

void getPosition(enet_uint8 const* src, glm::vec3& pos)
{
	memcpy(&pos.x, src + sizeof(float) * 0, sizeof(float));
	memcpy(&pos.y, src + sizeof(float) * 1, sizeof(float));
	memcpy(&pos.z, src + sizeof(float) * 2, sizeof(float));
}

