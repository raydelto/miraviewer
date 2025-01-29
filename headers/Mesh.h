//-----------------------------------------------------------------------------
// Mesh.h by Steve Jones 
// Copyright (c) 2015-2019 Game Institute. All Rights Reserved.
//
// Basic Mesh class
//-----------------------------------------------------------------------------
#pragma once

#include <vector>
#include <string>
#ifdef __APPLE__
#include <glad/glad.h>
#else
#define GLEW_STATIC
#include "GL/glew.h"	// Important - this header must come before glfw3 header
#endif
#include "glm/glm.hpp"


struct Vertex
{
	glm::vec3 position;
	glm::vec2 texCoords;
};

class Mesh
{
public:

	 Mesh();
	~Mesh();

    void loadModel(const std::string& filename);
	void draw();

private:

	void initBuffers();
    void processFaceVertex(const std::string& faceData, std::vector<unsigned int>& vertexIndices, std::vector<unsigned int>& uvIndices);

	bool mLoaded;
	std::vector<Vertex> mVertices;
	GLuint mVBO, mVAO;
};
