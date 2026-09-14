#version 330 core

out vec4 color;

in vec2 uvs;
in vec3 worldPos;
in vec3 tangent;
in vec3 bitangent;
in vec4 preColor;
//Only below 1 for transparent bricks, which are drawn with blending on
in float opacity;
in vec3 normal;
flat in int useDecal;

layout (std140) uniform BasicUniforms
{
	//Model Matrix:
	mat4 TranslationMatrix;
	mat4 RotationMatrix;
	mat4 ScaleMatrix;
	
	//Material uniforms:
	//These are -1 if not used, otherwise they point to what layer of their 2d texture array they are on
	int useAlbedo;
	int useNormal;
	int useMetalness;
	int useRoughness;
	int useHeight;
	int useAO;
	
	bool nonInstanced;
	bool cameraSpacePosition;
};

layout (std140) uniform CameraUniforms
{
	//Camera Uniforms:
	mat4 CameraProjection;
	mat4 CameraView;
	mat4 CameraAngle;
	vec3 CameraPosition;
	vec3 CameraDirection;
};

layout (std140) uniform EnvironmentUniforms
{
	//See EnvironmentUniforms in ShaderSpecification.h
	vec3 SunDirection;
	float FogDistanceMin;
	vec3 LightDirection;
	float FogDistanceMax;
	vec3 LightColor;
	float WaveTime;
	vec3 SkyColor;
	float WaterLevel;
	vec3 FogColor;
	float HorizonHeight;
	vec4 ClipPlane;
	vec3 AmbientColor;
	float ShadowStrength;
};

uniform sampler2DArray PBRArray;
uniform sampler2DArray DecalArray;
uniform sampler2DArrayShadow ShadowArray;

//Sun or moon view of each shadow cascade, nearest first, see Camera::calculateLightSpaceMatricies
uniform mat4 lightSpaceMatricies[3];

//graphics/shadowsoftness: 0 is one filtered sample, 1 to 3 are 3x3, 5x5 and 7x7 texel filters
uniform int shadowSoftness;

//Depth of the transparent brick nearest the light, and the color light picks up passing through transparent bricks, per cascade
//Only used when coloredShadows is set, see graphics/shadowcolor
uniform sampler2DArrayShadow TintDepthArray;
uniform sampler2DArray TintColorArray;
uniform bool coloredShadows;

uniform bool debug;

//Unlit brightening on top of lighting, e.g. the ghost brick's pulse, 0 (the default) for none
uniform float glow;

//Start tutorial code
//https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/6.pbr/1.2.lighting_textured/1.2.pbr.fs
const float PI = 3.14159265359;
// ----------------------------------------------------------------------------
// Easy trick to get tangent-normals to world-space to keep PBR code simplified.
// Don't worry if you don't get what's going on; you generally want to do normal 
// mapping the usual way for performance anways; I do plan make a note of this 
// technique somewhere later in the normal mapping tutorial.
vec3 getNormalFromMapGrad(vec2 realUV,vec2 dx,vec2 dy)
{
    //Extra code so that if useNormal(map) is 0, texture map normal defaults to 0,1,0 aka it's just the interpolated vertex normal
	vec3 tangentNormal = vec3(0,1,0);
	if(useNormal != -1)
		tangentNormal = textureGrad(PBRArray, vec3(realUV,useNormal) ,dx,dy).xyz * 2.0 - 1.0;
	else
		return normal;
	
	mat3 TBN = mat3(
		normalize(tangent),
		normalize(bitangent),
		normalize(normal)
	);

    return normalize(TBN * tangentNormal);
}
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(float NdotV,float NdotL, float roughness)
{
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   
//End tutorial code

uniform float test;

//Per axis sample offsets (in texels) and weights for filterShadow, s is how far into its texel the position is
//level 1 to 3 is a 3x3, 5x5 or 7x7 texel filter
//Castaño's optimized PCF from The Witness, see SampleShadowMapOptimizedPCF in https://github.com/TheRealMJP/Shadows
int shadowFilterAxis(float s, int level, out vec4 weight, out vec4 offset)
{
	if(level == 1)
	{
		weight.xy = vec2(3.0 - 2.0 * s, 1.0 + 2.0 * s);
		offset.xy = vec2((2.0 - s) / weight.x - 1.0, s / weight.y + 1.0);
		return 2;
	}
	if(level == 2)
	{
		weight.xyz = vec3(4.0 - 3.0 * s, 7.0, 1.0 + 3.0 * s);
		offset.xyz = vec3((3.0 - 2.0 * s) / weight.x - 2.0, (3.0 + s) / weight.y, s / weight.z + 2.0);
		return 3;
	}
	weight = vec4(5.0 * s - 6.0, 11.0 * s - 28.0, -(11.0 * s + 17.0), -(5.0 * s + 1.0));
	offset = vec4((4.0 * s - 5.0) / weight.x - 3.0, (4.0 * s - 16.0) / weight.y - 1.0, -(7.0 * s + 5.0) / weight.z + 1.0, -s / weight.w + 3.0);
	return 4;
}

//How much of the light reaches coords (xy in the map, z its depth) in one cascade, 1 for fully lit
//Every sample is a hardware filtered 2x2 comparison, so the filter is smooth rather than a grid of hard edges
float filterShadow(sampler2DArrayShadow map, vec3 coords, int layer, int level)
{
	if(level <= 0)
		return texture(map, vec4(coords.xy, layer, coords.z));

	vec2 mapSize = vec2(textureSize(map, 0).xy);
	vec2 uv = coords.xy * mapSize;
	vec2 base = floor(uv + 0.5);
	vec2 into = uv + 0.5 - base;
	base -= 0.5;

	vec4 uWeight, uOffset, vWeight, vOffset;
	int taps = shadowFilterAxis(into.x, level, uWeight, uOffset);
	shadowFilterAxis(into.y, level, vWeight, vOffset);

	float lit = 0.0;
	float total = 0.0;
	for(int x = 0; x < taps; x++)
	{
		for(int y = 0; y < taps; y++)
		{
			float weight = uWeight[x] * vWeight[y];
			lit += weight * texture(map, vec4((base + vec2(uOffset[x], vOffset[y])) / mapSize, layer, coords.z));
			total += weight;
		}
	}

	return lit / total;
}

//World size of one texel in a cascade. The light matrices are orthographic, so their first row is a world axis scaled by 2 / cascade width
float shadowTexelWorldSize(int cascade, float mapSize)
{
	mat4 light = lightSpaceMatricies[cascade];
	return 2.0 / (length(vec3(light[0][0], light[1][0], light[2][0])) * mapSize);
}

//How much light, and of what color, reaches this surface according to one cascade, 1 for fully lit and untinted
//edgeDistance is how far inside the cascade's edges the surface is, as a fraction of its width, and negative if it's outside
vec3 cascadeLight(int cascade, vec3 surfaceNormal, float grazing, out float edgeDistance)
{
	float mapSize = float(textureSize(ShadowArray, 0).x);
	float texelWorldSize = shadowTexelWorldSize(cascade, mapSize);

	//Farther cascades have bigger texels, so they get smaller filters, keeping soft edges about as wide in the world as in the nearest one
	float filterWidth = float(shadowSoftness * 2 + 1) * shadowTexelWorldSize(0, mapSize) / texelWorldSize;
	int level = clamp(int(floor((filterWidth - 1.0) * 0.5 + 0.5)), 0, shadowSoftness);
	float reach = float(level + 1);

	//Surfaces tilted away from the light need to be lifted further off themselves before the filter's outer samples stop hitting them
	vec3 offsetPos = worldPos + surfaceNormal * texelWorldSize * (0.5 + reach * grazing);
	vec3 coords = (lightSpaceMatricies[cascade] * vec4(offsetPos, 1.0)).xyz * 0.5 + 0.5;

	vec2 inside = min(coords.xy, 1.0 - coords.xy) - reach / mapSize;
	edgeDistance = min(inside.x, inside.y);
	if(edgeDistance < 0.0)
		return vec3(1.0);

	//Casters nearer the light than a cascade are flattened onto its near plane by GL_DEPTH_CLAMP, see LoopClient::renderEverything
	coords.z = clamp(coords.z, 0.0, 1.0);
	float lit = filterShadow(ShadowArray, coords, cascade, level);
	if(!coloredShadows || lit <= 0.0)
		return vec3(lit);

	//Only the part of the filter behind a transparent brick picks up its color
	float behindTransparent = 1.0 - filterShadow(TintDepthArray, coords, cascade, level);
	vec3 tint = texture(TintColorArray, vec3(coords.xy, cascade)).rgb;
	return lit * mix(vec3(1.0), tint, behindTransparent);
}

//Lights placed by Lua, nearest the camera first, see PointLights::update and PointLightUniforms in ShaderSpecification.h
layout (std140) uniform PointLightUniforms
{
	int PointLightCount;
	//World size of one shadow map texel per unit of distance along a cube face's axis
	float PointShadowTexelScale;
	//xyz position, w how far the light reaches
	vec4 PointLightPositionRange[32];
	//rgb color times brightness, a the light's shadow slot, -1 for none
	vec4 PointLightColorShadow[32];
	//xyz which way a spotlight points, w cosine of half its cone angle, below -1 for lights that shine every way
	vec4 PointLightSpotDirection[32];
	//Six cube faces per shadow slot: +x, -x, +y, -y, +z, -z
	mat4 PointShadowMatrices[48];
};

//Six layers per shadow slot, in the same order as PointShadowMatrices
uniform sampler2DArrayShadow PointShadowArray;

//Same as TintDepthArray and TintColorArray, a layer per cube face like PointShadowArray, only used when coloredShadows is set
uniform sampler2DArrayShadow PointTintDepthArray;
uniform sampler2DArray PointTintColorArray;

//How much of a shadowed point light, and of what color, reaches this surface, 1 for fully lit and untinted
vec3 pointLightShadow(int slot, vec3 fromLight, vec3 surfaceNormal, float lightFacing)
{
	vec3 axisDistance = abs(fromLight);
	int face;
	if(axisDistance.x >= axisDistance.y && axisDistance.x >= axisDistance.z)
		face = fromLight.x > 0.0 ? 0 : 1;
	else if(axisDistance.y >= axisDistance.z)
		face = fromLight.y > 0.0 ? 2 : 3;
	else
		face = fromLight.z > 0.0 ? 4 : 5;
	int layer = slot * 6 + face;

	//Texels get bigger farther from the light, so farther surfaces are lifted further off themselves
	float texelWorldSize = PointShadowTexelScale * max(axisDistance.x, max(axisDistance.y, axisDistance.z));
	int level = min(shadowSoftness, 1);
	float grazing = sqrt(1.0 - lightFacing * lightFacing);
	vec3 offsetPos = worldPos + surfaceNormal * texelWorldSize * (0.5 + float(level + 1) * grazing);

	vec4 lightClip = PointShadowMatrices[layer] * vec4(offsetPos, 1.0);
	vec3 coords = lightClip.xyz / lightClip.w * 0.5 + 0.5;
	coords.z = clamp(coords.z, 0.0, 1.0);
	float lit = filterShadow(PointShadowArray, coords, layer, level);
	if(!coloredShadows || lit <= 0.0)
		return vec3(lit);

	//One hardware filtered sample, the tint map is half resolution and already soft
	float behindTransparent = 1.0 - texture(PointTintDepthArray, vec4(coords.xy, layer, coords.z));
	vec3 tint = texture(PointTintColorArray, vec3(coords.xy, layer)).rgb;
	return lit * mix(vec3(1.0), tint, behindTransparent);
}

//Light from every point light that reaches this surface: inverse square falloff, eased to exactly nothing at each light's range
vec3 pointLighting(vec3 N, vec3 V, float NdotV, vec3 albedo, vec3 mor, vec3 F0, vec3 surfaceNormal)
{
	vec3 total = vec3(0.0);
	for(int i = 0; i < PointLightCount; i++)
	{
		vec3 toLight = PointLightPositionRange[i].xyz - worldPos;
		float distanceSquared = dot(toLight, toLight);
		float range = PointLightPositionRange[i].w;
		if(distanceSquared >= range * range)
			continue;

		float lightDistance = sqrt(distanceSquared);
		vec3 L = toLight / max(lightDistance, 0.0001);
		float NdotL = max(dot(N, L), 0.0);
		float lightFacing = dot(surfaceNormal, L);
		if(NdotL <= 0.0 || lightFacing <= 0.0)
			continue;

		float edge = distanceSquared / (range * range);
		float window = clamp(1.0 - edge * edge, 0.0, 1.0);
		float attenuation = window * window / (distanceSquared + 1.0);

		//Spotlights fade out over the outer part of their cone, see spotFactor in PointLights.cpp
		float spotCosine = PointLightSpotDirection[i].w;
		if(spotCosine > -1.5)
		{
			attenuation *= smoothstep(spotCosine, spotCosine + (1.0 - spotCosine) * 0.25, dot(-L, PointLightSpotDirection[i].xyz));
			if(attenuation <= 0.0)
				continue;
		}

		vec3 lit = vec3(1.0);
		int slot = int(floor(PointLightColorShadow[i].a + 0.5));
		if(slot >= 0)
		{
			lit = pointLightShadow(slot, -toLight, surfaceNormal, lightFacing);
			if(max(lit.r, max(lit.g, lit.b)) <= 0.0)
				continue;
		}

		vec3 H = normalize(V + L);
		float NDF = DistributionGGX(N, H, mor.b);
		float G = GeometrySmith(NdotV, NdotL, mor.b);
		vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
		vec3 specular = NDF * G * F / (4.0 * NdotV * NdotL + 0.001);
		vec3 kD = (vec3(1.0) - F) * (1.0 - mor.r);

		total += (kD * albedo / PI + specular) * PointLightColorShadow[i].rgb * attenuation * NdotL * lit;
	}
	return total;
}

void main()
{			
	vec2 dxuv = dFdx(uvs);
	vec2 dyuv = dFdy(uvs);
	
	vec3 viewVector = normalize(CameraPosition - worldPos);
	
	vec4 albedo_ = vec4(1,1,1,1);
	if(useAlbedo != -1)
		albedo_ = textureGrad(PBRArray,vec3(uvs,useAlbedo),dxuv,dyuv);
		
	if(useDecal != -1)
	{
		vec4 decalAlbedo = textureGrad(DecalArray,vec3(uvs,useDecal),dxuv,dyuv);
		albedo_ = mix(albedo_,decalAlbedo,decalAlbedo.a);
	}

	float nonLinearAlbedoF = 1.0;											
	vec3 albedo = pow(albedo_.rgb,vec3(1.0 + 1.2 * nonLinearAlbedoF));
	albedo = mix(albedo.rgb,preColor.rgb,preColor.a);
	
	vec3 newNormal = getNormalFromMapGrad(uvs,dxuv,dyuv);
		
	//Sun during the day, moon at night
	vec3 sunDirection = LightDirection;
	vec3 sunColor = LightColor;
	
	//Faces turned away from the light get no direct light anyway, sampling the shadow map there just adds acne
	vec3 sunlight = vec3(0.0);
	vec3 surfaceNormal = normalize(normal);
	float lightFacing = dot(surfaceNormal, sunDirection);
	if(lightFacing > 0.0)
	{
		float grazing = sqrt(1.0 - lightFacing * lightFacing);

		//Share of a cascade's width, along its edges, spent fading into the next cascade so the change in detail isn't a visible line
		const float blendBand = 0.1;

		vec3 lit = vec3(1.0);
		for(int i = 0; i<3; i++)
		{
			float edgeDistance;
			vec3 cascadeLit = cascadeLight(i, surfaceNormal, grazing, edgeDistance);
			if(edgeDistance < 0.0)
				continue;

			lit = cascadeLit;
			if(edgeDistance < blendBand)
			{
				//Nothing past the last cascade is shadowed, though that's only ever deep in the fog
				vec3 nextLit = vec3(1.0);
				if(i < 2)
				{
					float nextEdgeDistance;
					nextLit = cascadeLight(i + 1, surfaceNormal, grazing, nextEdgeDistance);
				}
				lit = mix(nextLit, cascadeLit, smoothstep(0.0, blendBand, edgeDistance));
			}
			break;
		}

		sunlight = lit;
	}

	//color = vec4(uvs.x,uvs.y,0,1);
	//color = vec4(normal,1);
	//return;
	
	vec3 mor = vec3(0,0,0.5);
	
	int morLayer = max(max(useMetalness,useRoughness),useAO);
		
	if(morLayer != -1)
		mor = textureGrad(PBRArray,vec3(uvs,morLayer),dxuv,dyuv).rgb;
	
	if(useMetalness == -1)
		mor.r = 0;
	if(useAO == -1)
		mor.g = 1;
	if(useRoughness == -1)
		mor.b = 0.5;
	
	float NdotV = max(dot(newNormal, viewVector), 0.0);	
	vec3 halfVector = normalize(viewVector + sunDirection);     
	vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, mor.r);
    vec3 F = fresnelSchlickRoughness(NdotV, F0, mor.b);
	vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - mor.r;
	
	float NdotL = max(dot(normalize(newNormal), normalize(sunDirection)), 0.0);  
	float NDF = DistributionGGX(newNormal, halfVector, mor.b);   
	float G   = GeometrySmith(NdotV,NdotL,mor.b);   
	
	vec3 numerator    = NDF * G * F; 
	float denominator = 4 * NdotV * NdotL + 0.001; // 0.001 to prevent divide by zero
	vec3 specular = numerator / denominator;
	
	vec3 shadowLight = clamp(sunlight, 0.35, 1.0);
	color.rgb = (kD * albedo / PI + specular) * sunColor.rgb * NdotL * shadowLight;
	//Ambient stays on while the direct light fades out at the horizon, and stops being shadowed there too, since the
	//shadow maps are about to switch between the sun and moon
	vec3 ambientShadow = mix(vec3(1.0), shadowLight, ShadowStrength);
	color.rgb += mor.g * albedo * AmbientColor * ambientShadow;
	color.rgb += pointLighting(newNormal, viewVector, NdotV, albedo, mor, F0, surfaceNormal);
	color.a = opacity;

	//Tone maping
	color.rgb = color.rgb / (color.rgb + vec3(1.0));
	//Gamma correction
	color.rgb = pow(color.rgb, vec3(1.0/2.2));

	//After tone mapping, which would otherwise squash the glow to almost nothing on bright or sunlit surfaces
	color.rgb = mix(color.rgb, vec3(1.0), glow);

	float fogFactor = clamp((length(CameraPosition - worldPos) - FogDistanceMin) / (FogDistanceMax - FogDistanceMin), 0.0, 1.0);
	color.rgb = mix(color.rgb, FogColor, fogFactor);
}


 



















