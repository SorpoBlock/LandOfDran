#pragma once

#include "../LandOfDran.h"

#include <glm/gtx/matrix_decompose.hpp>
#include <assimp/scene.h>

void printVec3(const glm::vec3& vec);
void printQuat(const glm::quat& quat);
glm::quat getRotationFromMatrix(const glm::mat4& in);

glm::vec3 lerp(glm::vec3 x, glm::vec3 y, float t);

//Calls glm::decompose
glm::vec3 getTransformFromMatrix(const glm::mat4 &in);

//assimp to opengl conversion for 4x4 matrices
void CopyaiMat(const aiMatrix4x4 &from, glm::mat4& to);

/*
	Returns a VAO representing two triangles in a quad with coords from -1 to 1
	It has 3d verticies and UVs
*/
GLuint createQuadVAO();

/*
	Pops all OpenGL and SDL errors in a loop and prints them to error
*/
void printAllGraphicsErrors();

