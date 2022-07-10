#ifndef __OBJMODEL_H__
#define __OBJMODEL_H__

#include "glew/glew.h"

class ObjModel
{
public:
    ObjModel(/* args */);
    ~ObjModel();

    int  InitModel(const char* modelPath);
    void Bind(GLuint posLoc);
    void Bind(GLuint posLoc, GLuint texLoc);
    void Bind(GLuint posLoc, GLuint texLoc, GLuint normalLoc); // 绑定 VertexData
    void Draw();

public:
    GLuint mVBO, mIBO;
    unsigned int mIndexCount;
};




#endif // __OBJMODEL_H__
