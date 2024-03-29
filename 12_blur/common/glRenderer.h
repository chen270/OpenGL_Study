#ifndef __GLRENDERER_H__
#define __GLRENDERER_H__

#include <windows.h>
#include "glew/glew.h"
#include <gl/GL.h>
#include "fbo.h"
#include "fullscreenQuad.h"

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

    int    InitFullScreenQuad();
    int    FullScreenQuadFun(HWND hwnd, HDC dc);
    int    LightAmbientEnv(HWND hwnd, HDC dc);
    int    LightDiffuseVertexEnv(HWND hwnd, HDC dc);
    int    LightDiffusePixelEnv(HWND hwnd, HDC dc);
    int    LightSpecularVertexEnv(HWND hwnd, HDC dc);
    int    LightSpecularPixelEnv(HWND hwnd, HDC dc, int viewW, int viewH);
    int    AllLightSource(HWND hwnd, HDC dc, int viewW, int viewH);
    int    UseXRay(HWND hwnd, HDC dc, int viewW, int viewH);
    int    Erosion_Dilation_Guassian(HWND hwnd, HDC dc, int viewW, int viewH);
    int    Erosion_Dilation_MultiGuassian(HWND hwnd, HDC dc, int viewW, int viewH);
    int    MultiGuassianSimplified(HWND hwnd, HDC dc, int viewW, int viewH);

    int    CartoonVertEnv(HWND hwnd, HDC dc);
    int    CartoonPixelEnv(HWND hwnd, HDC dc);
    

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
    FBO* mFBO;
    FullScreenQuad* mFullScreenQuad;

    GLuint mMainTex;
    GLuint mMulTex1;
};

#endif // __GLRENDERER_H__
