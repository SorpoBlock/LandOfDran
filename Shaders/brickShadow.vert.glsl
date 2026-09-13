#version 330 core

//Same inputs as brick.vert, used with modelShadow.geom which only needs world positions
layout(location = 0) in vec3 CubePosition;
layout(location = 5) in vec3 BrickCorner;
layout(location = 6) in vec3 BrickSize;

//STUD_SIZE and PLATE_SIZE in Bricks/Brick.h
const vec3 gridScale = vec3(1.0, 0.4, 1.0);

out vec3 worldPos;

void main()
{
	worldPos = (BrickCorner + CubePosition * BrickSize) * gridScale;
	gl_Position = vec4(worldPos, 1.0);
}
