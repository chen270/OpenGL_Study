#include "objModel.h"
#include <stdio.h>
#include "misc.h"

ObjModel::ObjModel(/* args */)
{
}

ObjModel::~ObjModel()
{
}

int ObjModel::InitModel(const char* modelPath)
{
    // 1.load model
    // load vertexes, vertex count, indexes, index count;
    unsigned int* indexData = nullptr;
    int vertexCount = 0, indexCount = 0;
    VertexData* vertexData = misc::LoadObjModel(modelPath, &indexData, vertexCount, indexCount);
    if (nullptr == vertexData) {
        printf("load obj model failed\n");
        return -1;
    }

    // 2.设置VBO IBO
    glGenBuffers(1, &mVBO);
    glGenBuffers(1, &mIBO);

    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VertexData) * vertexCount, vertexData, GL_STATIC_DRAW); // 传入显卡
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // 用glBufferData把索引复制到缓冲里
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indexCount, indexData, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    GL_CHECK_ERROR;

    delete[] indexData;
    delete[] vertexData;
    mIndexCount = indexCount;

    return 0;
}

void ObjModel::Bind(GLuint posLoc, GLuint texcoordLoc, GLuint normalLoc)
{
    glBindBuffer(GL_ARRAY_BUFFER, this->mVBO);
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), reinterpret_cast<void*>(0)); // 每个点间隔VertexData大小,从0开始

    glEnableVertexAttribArray(texcoordLoc);
    glVertexAttribPointer(texcoordLoc, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), reinterpret_cast<void*>(sizeof(float) * 3));

    //glEnableVertexAttribArray(normalLoc);
    //glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), reinterpret_cast<void*>(sizeof(float) * 5));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_ERROR;
}

void ObjModel::Draw()
{
    // IBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->mIBO);
    glDrawElements(GL_TRIANGLES, mIndexCount, GL_UNSIGNED_INT, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    GL_CHECK_ERROR;
}