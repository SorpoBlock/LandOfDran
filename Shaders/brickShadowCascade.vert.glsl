#version 330 core

//Same inputs as brick.vert, draws straight into one shadow cascade, see InstancedBrickRenderer::renderShadowCascade
layout(location = 0) in vec3 CubePosition;
layout(location = 5) in vec3 BrickCorner;
layout(location = 6) in vec3 BrickSize;
layout(location = 7) in vec4 BrickColor;
layout(location = 8) in vec4 VertexColor;
layout(location = 9) in float BrickAngle;

uniform mat4 lightSpaceMatrix;

//Bricks less opaque than this are skipped: half for plain shadows, 0 for the colored shadow passes where every transparent brick tints
uniform float minOpacity;

//Drawing special bricks' shapes, see brick.vert
uniform bool specialMesh;

//Only used by shadowTint.frag
out vec4 casterColor;

//STUD_SIZE and PLATE_SIZE in Bricks/Brick.h
const vec3 gridScale = vec3(1.0, 0.4, 1.0);

void main()
{
	casterColor = BrickColor;
	if(specialMesh && VertexColor.a >= 0.01)
		casterColor = vec4(VertexColor.rgb, VertexColor.a * BrickColor.a);

	//Collapsing every corner to one point leaves nothing to rasterize
	if(casterColor.a < minOpacity)
	{
		gl_Position = vec4(2.0, 2.0, 0.0, 1.0);
		return;
	}

	if(specialMesh)
	{
		float c = cos(BrickAngle * 1.5707963);
		float s = sin(BrickAngle * 1.5707963);
		mat3 turn = mat3(c, 0.0, -s, 0.0, 1.0, 0.0, s, 0.0, c);
		gl_Position = lightSpaceMatrix * vec4((BrickCorner + BrickSize * 0.5) * gridScale + turn * CubePosition, 1.0);
	}
	else
		gl_Position = lightSpaceMatrix * vec4((BrickCorner + CubePosition * BrickSize) * gridScale, 1.0);
}
