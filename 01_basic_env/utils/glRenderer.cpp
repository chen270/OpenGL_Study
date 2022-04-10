#include "glew/glew.h"
#include "glRenderer.h"
#include <stdio.h>

GlRenderer::GlRenderer(/* args */)
{
}

GlRenderer::~GlRenderer()
{
}

char* GlRenderer::LoadFileContent(const char* path)
{
	FILE* fp = fopen(path, "rb");
	if (fp != nullptr)
	{
		fseek(fp, 0, SEEK_END);
		int len = ftell(fp);
		char* buffer = new char[len + 1];
		rewind(fp);
		fread(buffer, len + 1, 1, fp);
		fclose(fp);
		return buffer;
	}

	return nullptr;
}


GLuint GlRenderer::CreateGPUProgram(const char *vsShaderPath, const char *fsShaderPath)
{
    //创建shader对象
	GLuint vsShader = glCreateShader(GL_VERTEX_SHADER);//创建引用ID存储 创建的着色器
	GLuint fsShader = glCreateShader(GL_FRAGMENT_SHADER);

	const char * vsCode = LoadFileContent(vsShaderPath);
	const char * fsCode = LoadFileContent(fsShaderPath);

	//ram->vram, 从内存传到显存
	glShaderSource(vsShader, 1, &vsCode, nullptr);//附加到着色器对象上
	glShaderSource(fsShader, 1, &fsCode, nullptr);

	//进行GPU上编译
	glCompileShader(vsShader);//检查错误方法见补充
	glCompileShader(fsShader);

	//绑定程序program
	GLuint program = glCreateProgram();
	glAttachShader(program, vsShader);
	glAttachShader(program, fsShader);

	//链接
	glLinkProgram(program);//检查错误方法见补充

    //在把着色器对象链接到程序对象以后，记得解绑定和删除着色器对象
	//解绑定
	glDetachShader(program, vsShader);
	glDetachShader(program, fsShader);

	glDeleteShader(vsShader);
	glDeleteShader(fsShader);

	return program;
}