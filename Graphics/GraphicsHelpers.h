#pragma once

#include "../LandOfDran.h"

/*
	Returns a VAO representing two triangles in a quad with coords from -1 to 1
	It has 3d verticies and UVs
*/
GLuint createQuadVAO();

/*
	Pops all OpenGL and SDL errors in a loop and prints them to error
*/
void printAllGraphicsErrors();