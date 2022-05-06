#include "glew/glew.h"
#include "glRenderer.h"
#include <stdio.h>
#include <iostream>
#include "Glm/glm.hpp" // 数学库，计算矩阵
#include "Glm/ext.hpp"

struct Vertex
{
	float pos[3];
	float color[4] = {1.0f,1.0f,1.0f,1.0f}; // 默认白色
};

GLRenderer::GLRenderer(/* args */)
{
	VAO = 0;
	VBO = 0;
	EBO = 0;

	vsCode = nullptr;
	fsCode = nullptr;
}

GLRenderer::~GLRenderer()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteProgram(program);
}

#define GL_CALL(x)      do{x; GLRenderer::CheckGLError(__FILE__, __LINE__);}while(0);
#define GL_CHECK_ERROR  GLRenderer::CheckGLError(__FILE__, __LINE__);
void GLRenderer::CheckGLError(const char * file, int line)
{
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        // const char* errStr = (char*)gluErrorString(error);//需要glut库函数
        const char* errStr = reinterpret_cast<const char*>(glewGetErrorString(error)); // glew库函数
        printf("glGetError = %d(0x%x), str = %s\n", error, error, errStr);
        switch (error)
        {
        case GL_INVALID_ENUM:
            printf("GL_INVALID_ENUM, %s: %d\n", file, line);
            break;
        case GL_INVALID_VALUE:
            printf("GL_INVALID_VALUE, %s: %d\n", file, line);
            break;
        case GL_INVALID_OPERATION:
            printf("GL_INVALID_OPERATION, %s: %d\n", file, line);
            break;
        case GL_STACK_OVERFLOW:
            printf("GL_STACK_OVERFLOW, %s: %d\n", file, line);
            break;
        case GL_STACK_UNDERFLOW:
            printf("GL_STACK_UNDERFLOW, %s: %d\n", file, line);
            break;
        case GL_OUT_OF_MEMORY:
            printf("GL_OUT_OF_MEMORY, %s: %d\n", file, line);
            break;
        default:
            printf("Unknown Error, %s: %d\n", file, line);
            break;
        }
    }
}

int GLRenderer::GLInit()
{
	/*
	* 初始化glew之前，需要一个OpenGL的环境，需要调用wglMakeCurrent
	  在初始化GLEW之前设置glewExperimental变量的值为GL_TRUE，
	  这样做能让GLEW在管理OpenGL的函数指针时更多地使用现代化的技术，
	  如果把它设置为GL_FALSE的话可能会在使用OpenGL的核心模式时出现一些问题。
	*/
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return -1;
	}
}

int GLRenderer::LoadFileContent(const char *path, char **buf)
{
	FILE *fp = fopen(path, "rb");
	if (fp != nullptr)
	{
		fseek(fp, 0, SEEK_END);
		int len = ftell(fp);
		char *buffer = new char[len + 1]();
		rewind(fp);
		fread(buffer, len + 1, 1, fp);
		fclose(fp);
		// return buffer;
		*buf = buffer;
	}

	return -1;
}

int GLRenderer::CheckCompile(GLuint vsShader)
{
	// check glCompileShader
	int success;
	glGetShaderiv(vsShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vsShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
		return -1;
	}

	return 0;
}

GLuint GLRenderer::CreateGPUProgram(const char *vsShaderPath, const char *fsShaderPath)
{
	// 创建shader对象
	GLuint vsShader = glCreateShader(GL_VERTEX_SHADER); // 创建引用ID存储 创建的着色器
	GLuint fsShader = glCreateShader(GL_FRAGMENT_SHADER);

	LoadFileContent(vsShaderPath, &vsCode);
	LoadFileContent(fsShaderPath, &fsCode);

	// ram->vram, 从内存传到显存
	glShaderSource(vsShader, 1, &vsCode, nullptr); // 附加到着色器对象上
	glShaderSource(fsShader, 1, &fsCode, nullptr);

	// 进行GPU上编译
	glCompileShader(vsShader); // 检查错误方法见补充
	CheckCompile(vsShader);

	glCompileShader(fsShader);
	CheckCompile(vsShader);

	// 绑定程序program
	this->program = glCreateProgram();
	glAttachShader(program, vsShader);
	glAttachShader(program, fsShader);

	// 链接
	glLinkProgram(program); // 检查错误方法见补充

	// check glLinkProgram
	//  检查 shader 语法问题
	int success = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success)
	{
		memset(infoLog, 0, sizeof(infoLog) / sizeof(char));
		glGetProgramInfoLog(program, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
		return -1;
	}

	// 在把着色器对象链接到程序对象以后，记得解绑定和删除着色器对象
	// 1.解绑定
	glDetachShader(program, vsShader);
	glDetachShader(program, fsShader);

	// 2.删除
	glDeleteShader(vsShader);
	glDeleteShader(fsShader);

	if (vsCode)
		delete[] vsCode;
	if (fsCode)
		delete[] fsCode;

	GL_CHECK_ERROR;
	return this->program;
}

int GLRenderer::initTriangle()
{
	Vertex vertex[3]; // 颜色默认白色
	vertex[0].pos[0] = 0.0f;
	vertex[0].pos[1] = 0.0f;
	vertex[0].pos[2] = -100.0f;

	vertex[1].pos[0] = 10.0f;
	vertex[1].pos[1] = 0.0f;
	vertex[1].pos[2] = -100.0f;

	vertex[2].pos[0] = 0.0f;
	vertex[2].pos[1] = 10.0f;
	vertex[2].pos[2] = -100.0f;

	unsigned int indexes[] = { 0,1,2 }; // 连接顺序0-1-2

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// 1. 绑定VAO
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	// 2. 把顶点数组复制到缓冲中供OpenGL使用
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 3, vertex, GL_STATIC_DRAW); // 3个点，传入显卡
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// 用glBufferData把索引复制到缓冲里
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexes), indexes, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	GL_CHECK_ERROR;
	return 0;
}

void GLRenderer::SetShaderQualifiers()
{
	// 单位矩阵
	float identity[] = {
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1
	};

	// 计算投影矩阵
	glm::mat4 projection = glm::perspective(45.0f, 800.0f / 600.0f, 0.1f, 1000.0f);

	GLint posLocation, colorLocation, MLocation, VLocation, PLocation;
	posLocation = glGetAttribLocation(program, "pos");
	colorLocation = glGetAttribLocation(program, "color"); // 不使用的话，可能会被编译器优化掉

	MLocation = glGetUniformLocation(program, "M");
	VLocation = glGetUniformLocation(program, "V");
	PLocation = glGetUniformLocation(program, "P");

	// 编译命令
	glUseProgram(program);
	glUniformMatrix4fv(MLocation, 1, GL_FALSE, identity);
	glUniformMatrix4fv(VLocation, 1, GL_FALSE, identity);
	glUniformMatrix4fv(PLocation, 1, GL_FALSE, glm::value_ptr(projection));

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glEnableVertexAttribArray(posLocation);
	glVertexAttribPointer(posLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(0)); // 每个点间隔Vertex大小,从0开始
	glEnableVertexAttribArray(colorLocation);
	glVertexAttribPointer(colorLocation, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(sizeof(float) * 3)); // 每个点间隔Vertex大小,起点是sizeof(float)*3即pos后面

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0); // 重置

	GL_CHECK_ERROR;
}

void GLRenderer::GetRendererObject(GLuint &vao, GLuint &vbo, GLuint &ebo)
{
	vao = this->VAO;
	vbo = this->VBO;
	ebo = this->EBO;
}
