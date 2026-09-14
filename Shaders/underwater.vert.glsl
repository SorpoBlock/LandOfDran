#version 330 core

out vec2 uv;

void main()
{
	//One oversized triangle covering the whole screen, no vertex buffer needed
	vec2 ndc = vec2(gl_VertexID == 1 ? 3.0 : -1.0, gl_VertexID == 2 ? 3.0 : -1.0);
	uv = ndc * 0.5 + 0.5;

	gl_Position = vec4(ndc, 0.0, 1.0);
}
