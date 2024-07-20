#version 330 core

out vec4 color;

in vec2 uvs;
in vec3 worldPos;
in vec3 tangent;
in vec3 bitangent;
in vec4 preColor;
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

uniform sampler2DArray PBRArray;
uniform sampler2DArray DecalArray;

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
		
	//Testing:
	vec3 sunDirection = normalize(vec3(0.2,1,0.4));
	vec3 sunColor = 15.0 * vec3(1,0.7,0.5);
	//End testing
	
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
	
	color.rgb = (kD * albedo / PI + specular) * sunColor.rgb * NdotL;
	color.rgb += mor.g * albedo * vec3(0.03);
	color.a = 1.0;
	
	//Tone maping
	color.rgb = color.rgb / (color.rgb + vec3(1.0));
	//Gamma correction
	color.rgb = pow(color.rgb, vec3(1.0/2.2));
}


 



















