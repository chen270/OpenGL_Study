#ifndef __OBJMODEL_H__
#define __OBJMODEL_H__

#include "glew/glew.h"

struct VertexData
{
    float position[3];
    float texcoord[2];
    float normal[3];
};


class ObjModel
{
public:
    ObjModel(/* args */);
    ~ObjModel();

    void InitModel(const char* modelPath);
    void Bind(GLuint posLoc, GLuint texLoc, GLuint normalLoc); // 绑定 VertexData
    void Draw();

public:
    GLuint mVBO, mIBO;
    unsigned int mIndexCount;

};




#endif // __OBJMODEL_H__