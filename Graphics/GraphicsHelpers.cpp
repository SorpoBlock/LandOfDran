#include "GraphicsHelpers.h"

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

