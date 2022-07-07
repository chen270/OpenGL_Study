#ifndef __GPUPROGRAM_H__
#define __GPUPROGRAM_H__

#include <stack>
#include <unordered_map>
#include <string>
#include <initializer_list>
#include "glew/glew.h"

class GPUProgram
{
public:
    GPUProgram(/* args */);
    ~GPUProgram();

    int AttachShader(GLenum shaderType, const char *shaderPath);
    int Link();
    GLuint GetGPUProgram();

    //void DetectAttributes(std::vector<std::string>attributeNames);
    void DetectAttributes(std::initializer_list<const char*>attributeNames);
    void DetectUniforms(std::initializer_list<const char*>uniformNames);
    GLint GetQualfiterLoc(const char* name);

    void DetectAttribute(const char *attributeName);
    void DetectUniform(const char*uniformName);

private:
    GLuint CompileShader(GLenum shaderType, const char *shaderPath);
    GLuint mProgram;
    std::stack<GLuint> mAttachShader;
    std::unordered_map<std::string, GLint> mQualfitersLoc;
};



#endif // __GPUPROGRAM_H__