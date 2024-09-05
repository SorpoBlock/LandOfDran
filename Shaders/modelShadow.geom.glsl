#version 330 core
  
layout(triangles) in;
layout(triangle_strip, max_vertices = 9) out;

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
    
uniform mat4 lightSpaceMatricies[3];
     
void main()
{      
	for (int invoc = 0; invoc<3; ++invoc)
	{
		for (int i = 0; i < 3; ++i)
		{
			gl_Position = lightSpaceMatricies[invoc] * gl_in[i].gl_Position;
			gl_Layer = invoc;
			EmitVertex();
		}
		EndPrimitive();
	
	}
}  
