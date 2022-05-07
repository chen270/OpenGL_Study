#ifndef __GLRENDERER_H__
#define __GLRENDERER_H__

#include <windows.h>
#include <gl/GL.h>

class GLRenderer
{
public:
    GLRenderer(/* args */);
    ~GLRenderer();
    
    int    GLInit();
    GLuint CreateGPUProgram(const char * vsShaderPath, const char * fsShaderPath);
    int    initTriangle();
    void   GetRendererObject(GLuint& vao, GLuint& vbo, GLuint& ebo);

private:
    int LoadFileContent(const char* path, char** buf);
    int CheckCompile(GLuint vsShader);

private:
    char infoLog[512];
    GLuint VBO, VAO, EBO;
    GLuint program;


    char* vsCode;
	char* fsCode;
};


#endif // __GLRENDERER_H__