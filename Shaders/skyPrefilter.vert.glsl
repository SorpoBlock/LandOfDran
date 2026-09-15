#version 330 core

//-1 to 1 across the cube map face being drawn into, see Skybox::prefilter
out vec2 facePosition;

void main()
{
	//One oversized triangle covering the whole face, no vertex buffer needed
	vec2 ndc = vec2(gl_VertexID == 1 ? 3.0 : -1.0, gl_VertexID == 2 ? 3.0 : -1.0);
	facePosition = ndc;
	gl_Position = vec4(ndc, 0.0, 1.0);
}
