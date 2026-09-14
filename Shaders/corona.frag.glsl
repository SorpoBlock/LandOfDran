#version 330 core

in vec2 offset;
in vec3 glowColor;

out vec4 color;

//Drawn with additive blending, see PointLights::renderCoronae
void main()
{
	float radius = length(offset);
	if(radius >= 1.0)
		discard;

	//A soft glow in the light's color around a small core that goes white
	float glow = pow(1.0 - radius, 2.5);
	float core = pow(max(1.0 - radius * 4.0, 0.0), 2.0);
	float strongest = max(glowColor.r, max(glowColor.g, glowColor.b));
	color = vec4(glowColor * glow + vec3(core * strongest * 0.6), 1.0);
}
