#include "gpuProgram.h"
#include "misc.h"
#include "glRenderer.h"
#include <iostream>

GPUProgram::GPUProgram(/* args */)
{
    this->program = glCreateProgram();
}

GPUProgram::~GPUProgram()
{
	glDeleteProgram(program);
}

GLuint GPUProgram::CompileShader(GLenum shaderType, const char *shaderPath)
{
    // 创建shader对象
	GLuint shader = glCreateShader(shaderType);

    char* shaderCode = nullptr;
    misc::LoadFileContent(shaderPath, &shaderCode);
    if(nullptr == shaderCode)
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
    if(GL_ZERO != shader)
    {
        // 绑定程序program
        glAttachShader(program, shader);
        mAttachShader.push(shader);
    }

    GL_CHECK_ERROR;
    return 0;
}

int GPUProgram::Link()
{
    // 链接
	glLinkProgram(program); // 检查错误方法见补充

    // check glLinkProgram
	//  检查 shader 语法问题
	int ret = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &ret);
    if (!ret)
    {
        std::cout << "Create GPU program, Link Error" << std::endl;
        GLint logLen;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
        char *infoLog = new char[logLen]();
        glGetProgramInfoLog(program, logLen, NULL, infoLog);
        std::cout << infoLog << std::endl;
        delete[] infoLog;

        glDeleteProgram(program);
        program = 0;
        return -1;
    }

    while (!mAttachShader.empty())
    {
        GLuint shader = mAttachShader.top();
        // 在把着色器对象链接到程序对象以后，记得解绑定和删除着色器对象
        // 1.解绑定
        glDetachShader(program, shader);
        // 2.删除
        glDeleteShader(shader);

        mAttachShader.pop();
    }

    GL_CHECK_ERROR;
    return 0;
}

GLuint GPUProgram::GetGPUProgram()
{
    return this->program;
}