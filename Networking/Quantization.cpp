#include "Quantization.h"

void addQuaternion(enet_uint8* dest, glm::quat quat)
{
	//First byte cleared in switch statement
	dest[1] = 0;
	dest[2] = 0;
	dest[3] = 0;

	//Look at the 4 components of the quaternion, and figure out which is the biggest and what sign it has
	int largestSign = quat[0] > 0 ? 1 : -1;
	int maxIndex = 0;
	float maxValue = fabs(quat[0]);

	for(int i = 0; i<4; i++)
	{
		if(fabs(quat[i]) > maxValue)
		{
			maxValue = quat[i];
			maxIndex = i;
			largestSign = quat[i] > 0 ? 1 : -1;
		}
	}

	//TODO: If maxValue ~= 1 then we could just signal (no) rotation with a single bit


	//Get the smaller 3 components in order
	float a = 0;
	float b = 0;
	float c = 0;

	//Setting dest[0] upper 2 bits encodes which of the the four components we are not sending
	switch (maxIndex)
	{
	case 0:
		a = quat[1];
		b = quat[2];
		c = quat[3];
		dest[0] = 0;
		break;
	case 1:
		a = quat[0];
		b = quat[2];
		c = quat[3];
		dest[0] = 64;
		break;
	case 2:
		a = quat[0];
		b = quat[1];
		c = quat[3];
		dest[0] = 128;
		break;
	case 3:
		a = quat[0];
		b = quat[1];
		c = quat[2];
		dest[0] = 192;
		break;
	}

	//This is so we do not need to transmit the sign of the largest component, if its neg. just invert everything else
	a *= largestSign;
	b *= largestSign;
	c *= largestSign;

	//Convert each to 9 bits, 10 including the sign
	unsigned short aInt = fabs(a) * 511;
	aInt |= a < 0 ? 512 : 0;
	unsigned short bInt = fabs(b) * 511;
	bInt |= b < 0 ? 512 : 0;
	unsigned short cInt = fabs(c) * 511;
	cInt |= c < 0 ? 512 : 0;

	//Upper 6 bits of A go to lower 6 bits of 0
	dest[0] |= (aInt >> 4);
	//Lower 4 bits of A go to upper 4 bits of 1
	dest[1] |= (aInt & 0b1111) << 4;

	//Upper 4 bits of B go to lower 4 bits of 1
	dest[1] |= (bInt >> 6);
	//Lower 6 bits of B go to upper 6 bits of 2
	dest[2] |= (bInt & 0b111111) << 2;

	//Upper 2 bits of C go to lower 2 bits of 2
	dest[2] |= (cInt >> 8);
	//Lower 8 bits of C are all of 3
	dest[3] |= (cInt & 0b11111111);
}

void getQuaternion(enet_uint8 const* src, glm::quat& quat)
{
	int largest = src[0] & 0b11000000;

	short rawA = (src[0] & 0b00111111) << 4;
	rawA |= (src[1] >> 6);

	short rawB = (src[1] & 0b00001111) << 6;
	rawB |= (src[2] >> 4);

	short rawC = (src[2] & 0b00000011) << 8;
	rawC |= src[3];

	float a = (rawA & 0b0111111111);
	a /= 511;
	a *= (rawA & 0b1000000000) ? -1 : 1;

	float b = (rawB & 0b0111111111);
	b /= 511;
	b *= (rawB & 0b1000000000) ? -1 : 1;

	float c = (rawC & 0b0111111111);
	c /= 511;
	c *= (rawC & 0b1000000000) ? -1 : 1;

	float d = sqrt(1.0f - (a * a + b * b + c * c));

	switch (largest)
	{
		case 0:
			quat[1] = a;
			quat[2] = b;
			quat[3] = c;
			quat[0] = d;

		break;
		case 64:
			quat[0] = a;
			quat[1] = d;
			quat[2] = b;
			quat[3] = c;

		break;
		case 128:
			quat[0] = a;
			quat[1] = b;
			quat[2] = d;
			quat[3] = c;

		break;
		case 192:
			quat[0] = a;
			quat[1] = b;
			quat[2] = c;
			quat[3] = d;

		break;
	}
}

void addPosition(enet_uint8* dest, const glm::vec3& pos)
{
	//TODO: Compression? Positions could be plausabily bounded between -/+2048 with 1/32s precision for 16 bits per component instead of 32
	//Bit less of a good trade than the quat compression though...
	memcpy(dest + sizeof(float) * 0, &pos.x, sizeof(float));
	memcpy(dest + sizeof(float) * 1, &pos.y, sizeof(float));
	memcpy(dest + sizeof(float) * 2, &pos.z, sizeof(float));
}

void getPosition(enet_uint8 const* src, glm::vec3& pos)
{
	memcpy(&pos.x, src + sizeof(float) * 0, sizeof(float));
	memcpy(&pos.y, src + sizeof(float) * 1, sizeof(float));
	memcpy(&pos.z, src + sizeof(float) * 2, sizeof(float));
}

void addVelocity(enet_uint8* dest, const glm::vec3& vel)
{
	//First byte stores sign in upper bit and left of the decimal value in lower 7 bits
	//Second byte stores right of the decimal value in 8 bits
	//Repeat for each component for 6 bytes
	int xLeft = floor(abs(vel.x));
	dest[0] = (vel.x < 0) ? 128 : 0;
	dest[0] |= std::min(xLeft,127);
	int xRight = (abs(vel.x) - xLeft) * 255;
	dest[1] = xRight;

	int yLeft = floor(abs(vel.y));
	dest[2] = (vel.y < 0) ? 128 : 0;
	dest[2] |= std::min(yLeft, 127);
	int yRight = (abs(vel.y) - yLeft) * 255;
	dest[3] = yRight;

	int zLeft = floor(abs(vel.z));
	dest[4] = (vel.z < 0) ? 128 : 0;
	dest[4] |= std::min(zLeft, 127);
	int zRight = (abs(vel.z) - zLeft) * 255;
	dest[5] = zRight;
}

void getVelocity(enet_uint8 const* src, glm::vec3& vel)
{
	vel.x = src[1];
	vel.x /= 255.0f;
	vel.x += (src[0] & 0b01111111);
	vel.x *= ((src[0] & 0b10000000) ? -1 : 1);

	vel.y = src[3];
	vel.y /= 255.0f;
	vel.y += (src[2] & 0b01111111);
	vel.y *= ((src[2] & 0b10000000) ? -1 : 1);

	vel.z = src[5];
	vel.z /= 255.0f;
	vel.z += (src[4] & 0b01111111);
	vel.z *= ((src[4] & 0b10000000) ? -1 : 1);
}

void addAngularVelocity(enet_uint8* dest, const glm::vec3& vel)
{

	int xLeft = floor(abs(vel.x));
	int xRight = (abs(vel.x) - xLeft) * 16;
	xLeft = std::min(xLeft, 63);

	int yLeft = floor(abs(vel.y));
	int yRight = (abs(vel.y) - yLeft) * 16;
	yLeft = std::min(yLeft, 63);

	int zLeft = floor(abs(vel.z));
	int zRight = (abs(vel.z) - zLeft) * 16;
	zLeft = std::min(zLeft, 63);

	dest[0] = xLeft << 2;			//6 bits
	dest[0] |= (xRight >> 2);		//2 bits, 0 filled
	dest[1] = (xRight & 0b11) << 6;	//2 bits
	dest[1] |= yLeft;				//6 bits, 1 filled
	dest[2] = yRight << 4;			//4 bits
	dest[2] |= zLeft >> 2;			//4 bits, 2 filled
	dest[3] = (zLeft & 0b11) << 6;	//2 bits
	dest[3] |= zRight;				//6 bits, 3 filled
}

void getAngularVelocity(enet_uint8 const* src, glm::vec3& vel)
{
	int xLeft = src[0] >> 2;
	int xRight = (src[0] & 0b11) << 2;
	xRight |= src[1] >> 6;

	int yLeft = src[1] & 0b00111111;
	int yRight = src[2] >> 4;

	int zLeft = (src[2] & 0b00001111) << 2;
	zLeft |= src[3] >> 6;
	int zRight = src[3] & 0b00111111;

	vel.x = xLeft + xRight / 16.0f;
	vel.y = yLeft + yRight / 16.0f;
	vel.z = zLeft + zRight / 16.0f;
}



