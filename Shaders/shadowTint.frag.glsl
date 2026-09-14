#version 330 core

//Combined into a cascade of the tint map with a per-channel minimum by LoopClient::renderEverything, see coloredShadows in model.frag
in vec4 casterColor;

out vec4 color;

void main()
{
	//Light through a transparent brick picks up its color and loses some brightness, more of both the more opaque the brick is
	vec3 transmitted = mix(vec3(1.0), casterColor.rgb, casterColor.a) * (1.0 - 0.5 * casterColor.a);
	color = vec4(transmitted, 1.0);
}
