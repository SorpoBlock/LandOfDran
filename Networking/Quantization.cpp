#include "Quantization.h"


// Function to encode a float into 16 bits
uint16_t encodeFloat(float value) {
	uint16_t result = 0;

	// Handle the sign (1 bit)
	bool sign = (value < 0);
	if (sign) {
		value = -value;  // Make the value positive for further processing
	}

	// Handle the integer part (9 bits)
	uint16_t integerPart = static_cast<uint16_t>(value);
	if (integerPart > 511) {
		integerPart = 511;  // Clamp the integer part to 9 bits
	}

	// Handle the fractional part (6 bits)
	float fractionalPart = value - integerPart;
	uint16_t fractionalBits = static_cast<uint16_t>(fractionalPart * 64);  // Convert to 6-bit precision

	// Assemble the result
	result |= (sign << 15);                  // 1 bit for sign
	result |= ((integerPart & 0x1FF) << 6);  // 9 bits for integer part
	result |= (fractionalBits & 0x3F);       // 6 bits for fractional part

	return result;
}

// Function to encode three floats into 6 bytes
void encodeThreeFloats(float f1, float f2, float f3, uint8_t* buffer) {
	uint16_t encodedF1 = encodeFloat(f1);
	uint16_t encodedF2 = encodeFloat(f2);
	uint16_t encodedF3 = encodeFloat(f3);

	// Place encoded floats into the 6-byte buffer
	buffer[0] = encodedF1 >> 8;
	buffer[1] = encodedF1 & 0xFF;

	buffer[2] = encodedF2 >> 8;
	buffer[3] = encodedF2 & 0xFF;

	buffer[4] = encodedF3 >> 8;
	buffer[5] = encodedF3 & 0xFF;
}

// Helper function to print the buffer in binary form
void printBuffer(const uint8_t* buffer, size_t length) {
	for (size_t i = 0; i < length; ++i) {
		std::bitset<8> bits(buffer[i]);
		std::cout << bits << " ";
	}
	std::cout << std::endl;
}


// Function to decode 16 bits into a float
float decodeFloat(uint16_t encodedValue) {
	// Extract the sign bit (1 bit)
	bool sign = (encodedValue >> 15) & 1;

	// Extract the integer part (9 bits)
	uint16_t integerPart = (encodedValue >> 6) & 0x1FF;

	// Extract the fractional part (6 bits)
	uint16_t fractionalBits = encodedValue & 0x3F;
	float fractionalPart = static_cast<float>(fractionalBits) / 64.0f;  // Convert 6 bits back to fractional part

	// Combine integer and fractional parts
	float value = integerPart + fractionalPart;

	// Apply the sign
	if (sign) {
		value = -value;
	}

	return value;
}

// Function to decode 6 bytes into three floats
void decodeThreeFloats(const uint8_t* buffer, float& f1, float& f2, float& f3) {
	// Read 16 bits for each float
	uint16_t encodedF1 = (buffer[0] << 8) | buffer[1];
	uint16_t encodedF2 = (buffer[2] << 8) | buffer[3];
	uint16_t encodedF3 = (buffer[4] << 8) | buffer[5];

	// Decode each 16-bit value into a float
	f1 = decodeFloat(encodedF1);
	f2 = decodeFloat(encodedF2);
	f3 = decodeFloat(encodedF3);
}


//The three smallest components of a quaternion can never be larger than sqrt(2)/2 so we multiply by this to normalize to 0.0-1.0 before turning into an integer
const float quatCompMulti = 1.414f;

/*
	This uses sends the three smallest components normalized from 0 to sqrt(2)/2 with an extra 2 bits to encode which component was largest
	A 4-float 128-bit quaternion is compressed to 32 bits
*/
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
		if(fabs(quat[i]) > fabs(maxValue))
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
	unsigned short aInt = fabs(a) * quatCompMulti * 511.f;
	unsigned short bInt = fabs(b) * quatCompMulti * 511.f;
	unsigned short cInt = fabs(c) * quatCompMulti * 511.f;

	aInt |= a < 0 ? 512 : 0;
	bInt |= b < 0 ? 512 : 0;
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
	//Which component was omitted
	int largest = src[0] & 0b11000000;

	//Get the raw integer bit values for the smaller 3 components
	short rawA = (src[0] & 0b00111111) << 4;
	rawA |= (src[1] >> 6);

	short rawB = (src[1] & 0b00001111) << 6;
	rawB |= (src[2] >> 2);

	short rawC = (src[2] & 0b00000011) << 8;
	rawC |= src[3];

	//Turn three smaller components from 0 - 511 to 0 to sqrt(2)/2
	float a = (rawA & 0b0111111111);
	a /= 511.f;
	a /= quatCompMulti;
	a *= (rawA & 0b1000000000) ? -1 : 1;

	float b = (rawB & 0b0111111111);
	b /= 511.f;
	b /= quatCompMulti;
	b *= (rawB & 0b1000000000) ? -1 : 1;

	float c = (rawC & 0b0111111111);
	c /= 511.f;
	c /= quatCompMulti;
	c *= (rawC & 0b1000000000) ? -1 : 1;

	//Reconstitute largest component from 3 smallest ones
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
	encodeThreeFloats(pos.x, pos.y, pos.z, dest);

	//TODO: Compression? Positions could be plausabily bounded between -/+2048 with 1/32s precision for 16 bits per component instead of 32
	//Bit less of a good trade than the quat compression though...
	/*memcpy(dest + sizeof(float) * 0, &pos.x, sizeof(float));
	memcpy(dest + sizeof(float) * 1, &pos.y, sizeof(float));
	memcpy(dest + sizeof(float) * 2, &pos.z, sizeof(float));*/
}

void getPosition(enet_uint8 const* src, glm::vec3& pos)
{
	decodeThreeFloats(src, pos.x, pos.y, pos.z);

	/*memcpy(&pos.x, src + sizeof(float) * 0, sizeof(float));
	memcpy(&pos.y, src + sizeof(float) * 1, sizeof(float));
	memcpy(&pos.z, src + sizeof(float) * 2, sizeof(float));*/
}

//Velocity is hereby bounded to -127 to 127 in each component with 1/255 precision
//TODO: Maybe have more bits for magnitude and less for precision
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

//Angular velocity is bounded to -63 to 63 in each component with 1/16 precision
//The assumption is that precision here just isn't that important for visuals, since rotation is updated separately 
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



