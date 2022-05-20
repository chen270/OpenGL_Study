#ifndef __GLRENDERER_H__
#define __GLRENDERER_H__

#include <windows.h>
#include <gl/GL.h>

class ShaderParameters;

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
    int    InitModel(ShaderParameters* sp, const GLuint& program);
    int    UpdateModel(ShaderParameters &sp, float &angle, const GLuint& program);

    static void CheckGLError(const char *file, int line);
    GLuint CreateTextureFromFile(const char *imagePath);

private:
    char infoLog[512];
    GLuint VBO, VAO, EBO;
    // GLuint program;

    char* vsCode;
	char* fsCode;
    float identity[16] = {
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1
	};
};

#define GL_CALL(x)      do{x; GLRenderer::CheckGLError(__FILE__, __LINE__);}while(0);
#define GL_CHECK_ERROR  GLRenderer::CheckGLError(__FILE__, __LINE__);

struct MVPMatrix;
class ShaderParameters
{
public:
    ShaderParameters(){
        posLoc = 0;
        texcoordLoc = 0;
        normalLoc = 0;
        MLoc = 0;
        VLoc = 0;
        PLoc = 0;
        NormalMatrixLoc = 0;
        textureLoc = 0;
        imgTex = 0;
        modelMsg.vertexCount = 0;
        modelMsg.indexCount = 0;
        modelMsg.vertexData = nullptr;
        modelMsg.indexData = nullptr;
        modelMsg.mvp = nullptr;
    }

    ~ShaderParameters(){
        posLoc = 0;
        texcoordLoc = 0;
        normalLoc = 0;
        MLoc = 0;
        VLoc = 0;
        PLoc = 0;
        NormalMatrixLoc = 0;
        textureLoc = 0;
        modelMsg.vertexCount = 0;
        modelMsg.indexCount = 0;
        if (modelMsg.vertexData != nullptr){
            delete[] modelMsg.vertexData;
            modelMsg.vertexData = nullptr;
        }
        if (modelMsg.indexData != nullptr){
            delete[] modelMsg.indexData;
            modelMsg.indexData = nullptr;
        }

        if (modelMsg.mvp != nullptr)
        {
            delete modelMsg.mvp;
            modelMsg.mvp = nullptr;
        }

        if (imgTex > 0)
        {
            glDeleteTextures(1, &imgTex);
            imgTex = 0;
        }
    }

public:
    GLint posLoc;
    GLint texcoordLoc;
    GLint normalLoc;
    GLint MLoc;
    GLint VLoc;
    GLint PLoc;
    GLint NormalMatrixLoc;
    GLint textureLoc;
    GLuint imgTex; // 纹理

    struct ModelMsg
    {
        int vertexCount;
        int indexCount;
        void *vertexData;
        void *indexData;
		MVPMatrix* mvp;
    } modelMsg;
};

#endif // __GLRENDERER_H__