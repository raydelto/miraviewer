//-----------------------------------------------------------------------------
// Texture2D.h by Steve Jones
// Copyright (c) 2015-2019 Game Institute. All Rights Reserved.
//
// Simple 2D texture class
//-----------------------------------------------------------------------------
#pragma once

#ifdef __APPLE__
#include <glad/glad.h>
#else
#define GLEW_STATIC
#include "GL/glew.h" // Important - this header must come before glfw3 header
#endif
#include <string>
using std::string;

class Texture2D
{
public:
	Texture2D();
	virtual ~Texture2D();

	bool loadTexture(const string &fileName, bool generateMipMaps = true);
	void bind(GLuint texUnit = 0);
	void unbind(GLuint texUnit = 0);

private:
	Texture2D(const Texture2D &rhs) {}
	Texture2D &operator=(const Texture2D &rhs) {}

	GLuint mTexture;
};
