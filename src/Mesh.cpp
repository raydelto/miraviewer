//-----------------------------------------------------------------------------
// Mesh.cpp by Steve Jones
// Copyright (c) 2015-2019 Game Institute. All Rights Reserved.
//
// Basic Mesh class
//-----------------------------------------------------------------------------
#include "Mesh.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

Mesh::Mesh()
    : mLoaded(false), mVAO(0), mVBO(0), mEBO(0) // Initialize mVAO, mVBO, and mEBO
{
}

Mesh::~Mesh()
{
    glDeleteVertexArrays(1, &mVAO);
    glDeleteBuffers(1, &mVBO);
    glDeleteBuffers(1, &mEBO); // Delete EBO
}

void Mesh::loadModel(const std::string &path)
{
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }

    std::cout << "Number of meshes: " << scene->mNumMeshes << std::endl;
    for (unsigned int i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[i];

        size_t vertexOffset = mVertices.size(); // Keep track of base vertex index

        std::cout << "Mesh[" << i << "] Vertices: " << mesh->mNumVertices
                  << " Faces: " << mesh->mNumFaces << std::endl;

        for (unsigned int j = 0; j < mesh->mNumVertices; j++)
        {
            Vertex aVertex;

            aiVector3D vertex = mesh->mVertices[j];
            aVertex.position = glm::vec3(vertex.x, vertex.y, vertex.z);

            if (mesh->HasTextureCoords(0))
            {
                aiVector3D texCoord = mesh->mTextureCoords[0][j];
                aVertex.texCoords = glm::vec2(texCoord.x, texCoord.y);
            }

            mVertices.push_back(aVertex);
        }

        for (unsigned int j = 0; j < mesh->mNumFaces; j++)
        {
            aiFace face = mesh->mFaces[j];
            if (face.mNumIndices != 3)
            {
                std::cout << "Warning: Face " << j << " has " << face.mNumIndices
                          << " vertices (expected 3)" << std::endl;
                continue;
            }
            for (unsigned int k = 0; k < face.mNumIndices; k++)
            {
                unsigned int index = face.mIndices[k] + vertexOffset;
                if (index >= mVertices.size())
                {
                    std::cout << "Warning: Invalid index " << index << " at face " << j << std::endl;
                    continue;
                }
                mIndices.push_back(index);
            }
        }
    }

    if (mIndices.size() % 3 != 0)
    {
        std::cout << "Warning: Index count is not a multiple of 3!" << std::endl;
    }

    if (mVertices.empty() || mIndices.empty())
    {
        std::cerr << "ERROR::MESH::NO_VERTICES_OR_INDICES" << std::endl;
        return;
    }

    std::cout << "Model has been loaded correctly" << std::endl;
    mLoaded = true;
    initBuffers();
}

//-----------------------------------------------------------------------------
// Create and initialize the vertex buffer and vertex array object
// Must have valid, non-empty std::vector of Vertex objects.
//-----------------------------------------------------------------------------
void Mesh::initBuffers()
{
    glGenVertexArrays(1, &mVAO);
    glGenBuffers(1, &mVBO);
    glGenBuffers(1, &mEBO); // Generate EBO

    glBindVertexArray(mVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, mVertices.size() * sizeof(Vertex), &mVertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndices.size() * sizeof(unsigned int), &mIndices[0], GL_STATIC_DRAW);

    // Vertex Positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)0);

    // Vertex Texture Coords
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)(3 * sizeof(GLfloat)));

    // unbind to make sure other code does not change it somewhere else
    glBindVertexArray(0);
}

void Mesh::draw()
{
    if (!mLoaded)
        return;
    if (mIndices.empty())
    {
        std::cerr << "No indices to render!" << std::endl;
        return;
    }

    glBindVertexArray(mVAO);
    glDrawElements(GL_TRIANGLES, mIndices.size(), GL_UNSIGNED_INT, 0); // Use glDrawElements
    glBindVertexArray(0);
}
