#ifndef __GPUPROGRAM_H__
#define __GPUPROGRAM_H__

#include "glew/glew.h"
#include <stack>
#include <unordered_map>
#include <string>

class GPUProgram
{
public:
    GPUProgram(/* args */);
    ~GPUProgram();

    int AttachShader(GLenum shaderType, const char *shaderPath);
    int Link();
    GLuint GetGPUProgram();

    void DetectAttribute(const char * attributeName);
    void DetectUniform(const char * uniformName);

    std::unordered_map<std::string, GLint> mAttributeLoc;
    std::unordered_map<std::string, GLint> mUniformLoc;

private:
    GLuint CompileShader(GLenum shaderType, const char *shaderPath);
    GLuint program;
    std::stack<GLuint> mAttachShader;
};



#endif // __GPUPROGRAM_H__