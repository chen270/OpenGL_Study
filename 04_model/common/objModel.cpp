#include "objModel.h"

ObjModel::ObjModel(/* args */)
{
}

ObjModel::~ObjModel()
{
}

void ObjModel::InitModel(const char* modelPath)
{

}
    
void ObjModel::Bind(GLuint posLoc, GLuint texLoc, GLuint normalLoc)
{
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VertexData) * vertexCount, vertexData, GL_STATIC_DRAW); // 传入显卡
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // 用glBufferData把索引复制到缓冲里
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indexCount, indexData, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

}

void ObjModel::Draw()
{
    
}