#include "gpuProgram.h"
#include <iostream>
#include "misc.h"
#include "glRenderer.h"


GPUProgram::GPUProgram(/* args */)
{
    this->mProgram = glCreateProgram();
}

GPUProgram::~GPUProgram()
{
    glDeleteProgram(mProgram);
}

GLuint GPUProgram::CompileShader(GLenum shaderType, const char *shaderPath)
{
    // 创建shader对象
    GLuint shader = glCreateShader(shaderType);

    char* shaderCode = nullptr;
    misc::LoadFileContent(shaderPath, &shaderCode);
    if (nullptr == shaderCode)
    {
        std::cout << "can not load shader code from shader-file" << std::endl;
        return 0;
    }

    // ram->vram, 从内存传到显存
    glShaderSource(shader, 1, &shaderCode, nullptr); // 附加到着色器对象上

    delete[] shaderCode;

    // 进行GPU上编译
    glCompileShader(shader);

    // check compile
    GLint ret;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ret);
    if (GL_FALSE == ret)
    {
        std::cout << "Compile shader:" << shaderPath << ", failed:" << std::endl;
        GLint logLen;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        char *infoLog = new char[logLen]();
        glGetShaderInfoLog(shader, logLen, NULL, infoLog);
        std::cout << infoLog << std::endl;
        delete[] infoLog;
        glDeleteShader(shader);
        return 0;
    }

    GL_CHECK_ERROR;
    return shader;
}

int GPUProgram::AttachShader(GLenum shaderType, const char *shaderPath)
{
    GLuint shader = CompileShader(shaderType, shaderPath);
    if (GL_ZERO != shader)
    {
        // 绑定程序program
        glAttachShader(mProgram, shader);
        mAttachShader.push(shader);
    }

    GL_CHECK_ERROR;
    return 0;
}

int GPUProgram::Link()
{
    // 链接
    glLinkProgram(mProgram); // 检查错误方法见补充

    // check glLinkProgram
    //  检查 shader 语法问题
    int ret = 0;
    glGetProgramiv(mProgram, GL_LINK_STATUS, &ret);
    if (!ret)
    {
        std::cout << "Create GPU program, Link Error" << std::endl;
        GLint logLen;
        glGetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &logLen);
        char *infoLog = new char[logLen]();
        glGetProgramInfoLog(mProgram, logLen, NULL, infoLog);
        std::cout << infoLog << std::endl;
        delete[] infoLog;

        glDeleteProgram(mProgram);
        mProgram = 0;
        return -1;
    }

    while (!mAttachShader.empty())
    {
        GLuint shader = mAttachShader.top();
        // 在把着色器对象链接到程序对象以后，记得解绑定和删除着色器对象
        // 1.解绑定
        glDetachShader(mProgram, shader);
        // 2.删除
        glDeleteShader(shader);

        mAttachShader.pop();
    }

    GL_CHECK_ERROR;
    return 0;
}

GLuint GPUProgram::GetGPUProgram()
{
    return this->mProgram;
}

void GPUProgram::DetectAttributes(std::initializer_list<const char *> attributeNames)
{
    if (mProgram == GL_FALSE)
        return;

    GLint loc = 0;
    for (const auto &str : attributeNames)
    {
        loc = glGetAttribLocation(mProgram, str);
        if (loc < 0)
            continue;
        mQualfitersLoc.insert(std::pair<std::string, GLint>(str, loc));
    }
}

void GPUProgram::DetectUniforms(std::initializer_list<const char *> uniformNames)
{
    if (mProgram == GL_FALSE)
        return;

    GLint loc = 0;
    for (const auto &str : uniformNames)
    {
        loc = glGetUniformLocation(mProgram, str);
        if (loc < 0)
            continue;
        mQualfitersLoc.insert(std::pair<std::string, GLint>(str, loc));
    }
}

GLint GPUProgram::GetQualfiterLoc(const char *name)
{
    if (mProgram == GL_FALSE)
        return 0;

    auto iter = mQualfitersLoc.find(std::string(name));
    if (mQualfitersLoc.end() == iter)
        return 0;

    return iter->second;
}
