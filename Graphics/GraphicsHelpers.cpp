#include "GraphicsHelpers.h"

//assimp to opengl conversion for 4x4 matrices
void CopyaiMat(aiMatrix4x4 from, glm::mat4& to)
{
	to[0][0] = from.a1; to[1][0] = from.a2;
	to[2][0] = from.a3; to[3][0] = from.a4;
	to[0][1] = from.b1; to[1][1] = from.b2;
	to[2][1] = from.b3; to[3][1] = from.b4;
	to[0][2] = from.c1; to[1][2] = from.c2;
	to[2][2] = from.c3; to[3][2] = from.c4;
	to[0][3] = from.d1; to[1][3] = from.d2;
	to[2][3] = from.d3; to[3][3] = from.d4;
}

void printAllGraphicsErrors()
{
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR)
	{
		error("OpenGL error: " + std::to_string(err));
		error("Last SDL error string: " + std::string(SDL_GetError()));
	}
}

GLuint createQuadVAO()
{
	GLuint debugTriangleVAO;

	glGenVertexArrays(1, &debugTriangleVAO);
	glBindVertexArray(debugTriangleVAO);

	std::vector<glm::vec3> verts;
	verts.clear();
	verts.push_back(glm::vec3(-1, -1, 0));
	verts.push_back(glm::vec3(1, -1, 0));
	verts.push_back(glm::vec3(1, 1, 0));

	verts.push_back(glm::vec3(1, 1, 0));
	verts.push_back(glm::vec3(-1, 1, 0));
	verts.push_back(glm::vec3(-1, -1, 0));

	std::vector<glm::vec2> uv;
	uv.clear();
	uv.push_back(glm::vec2(0, 0));
	uv.push_back(glm::vec2(1, 0));
	uv.push_back(glm::vec2(1, 1));

	uv.push_back(glm::vec2(1, 1));
	uv.push_back(glm::vec2(0, 1));
	uv.push_back(glm::vec2(0, 0));

	GLuint vertexBuffer;
	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(glm::vec3), &verts[0][0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	GLuint uvBuffer;
	glGenBuffers(1, &uvBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
	glEnableVertexAttribArray(1);
	glBufferData(GL_ARRAY_BUFFER, uv.size() * sizeof(glm::vec2), &uv[0][0], GL_STATIC_DRAW);
	glVertexAttribPointer(
		1,                  // attribute, 0 = verticies
		2,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);


	glBindVertexArray(0);

	return debugTriangleVAO;
}

