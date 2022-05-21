#ifndef __GLRENDERER_H__
#define __GLRENDERER_H__

#include <windows.h>
#include "glew/glew.h"
#include <gl/GL.h>

class ShaderParameters;
class GPUProgram;
class ObjModel;
struct MVPMatrix;

class GLRenderer
{
public:
    GLRenderer(/* args */);
    ~GLRenderer();

    int    GLInit();
    // GLuint CreateGPUProgram(const char * vsShaderPath, const char * fsShaderPath);
    int    InitTriangle();
    void   GetRendererObject(GLuint& vao, GLuint& vbo, GLuint& ebo);
    void   SetTriangle_ShaderQualifiers(const GLuint& program);
    int    InitModel();
    int    UpdateModel(float &angle);

    static void CheckGLError(const char *file, int line);
    GLuint CreateTextureFromFile(const char *imagePath);

private:
    GPUProgram* gpuProgram;
    ObjModel*   objModel;
    GLuint      mProgram;
    char        infoLog[512];
    GLuint      VBO, VAO, EBO;
    // GLuint program;

    char* vsCode;
    char* fsCode;
    float identity[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };

    MVPMatrix* mModelMvp;
    GLuint mMainTex;
    GLuint mMulTex1;
};

#endif // __GLRENDERER_H__
