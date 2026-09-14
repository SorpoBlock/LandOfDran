#version 330 core

//Same inputs as brick.vert, draws straight into one shadow cascade, see InstancedBrickRenderer::renderShadowCascade
layout(location = 0) in vec3 CubePosition;
layout(location = 5) in vec3 BrickCorner;
layout(location = 6) in vec3 BrickSize;
layout(location = 7) in vec4 BrickColor;

uniform mat4 lightSpaceMatrix;

//Bricks less opaque than this are skipped: half for plain shadows, 0 for the colored shadow passes where every transparent brick tints
uniform float minOpacity;

//Only used by shadowTint.frag
out vec4 casterColor;

//STUD_SIZE and PLATE_SIZE in Bricks/Brick.h
const vec3 gridScale = vec3(1.0, 0.4, 1.0);

void main()
{
	casterColor = BrickColor;

	//Collapsing every corner to one point leaves nothing to rasterize
	if(BrickColor.a < minOpacity)
	{
		gl_Position = vec4(2.0, 2.0, 0.0, 1.0);
		return;
	}

	gl_Position = lightSpaceMatrix * vec4((BrickCorner + CubePosition * BrickSize) * gridScale, 1.0);
}
