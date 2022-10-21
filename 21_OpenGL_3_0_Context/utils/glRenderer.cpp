#include "glRenderer.h"
#include <stdio.h>
#include <iostream>

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

int GLRenderer::GLInit()
{
	/*
	* 初始化glew之前，需要一个OpenGL的环境，需要调用wglMakeCurrent
	  在初始化GLEW之前设置glewExperimental变量的值为GL_TRUE，
	  这样做能让GLEW在管理OpenGL的函数指针时更多地使用现代化的技术，
	  如果把它设置为GL_FALSE的话可能会在使用OpenGL的核心模式时出现一些问题。
	*/
	return 0;
}

int GLRenderer::LoadFileContent(const char* path, char** buf)
{
	FILE* fp = fopen(path, "rb");
	if (fp != nullptr)
	{
		fseek(fp, 0, SEEK_END);
		int len = ftell(fp);
		char* buffer = new char[len + 1]();
		rewind(fp);
		fread(buffer, len + 1, 1, fp);
		fclose(fp);
		//return buffer;
		*buf = buffer;
		return 0;
	}

	return -1;
}

int GLRenderer::CheckCompile(GLuint vsShader)
{
	//check glCompileShader
	int  success;
	glGetShaderiv(vsShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vsShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
		return -1;
	}

	return 0;
}


GLuint GLRenderer::CreateGPUProgram(const char *vsShaderPath, const char *fsShaderPath)
{
    //创建shader对象
	GLuint vsShader = glCreateShader(GL_VERTEX_SHADER);//创建引用ID存储 创建的着色器
	GLuint fsShader = glCreateShader(GL_FRAGMENT_SHADER);

	int ret = LoadFileContent(vsShaderPath, &vsCode);
	if (ret < 0)
	{
		std::cout << "vs shader LoadFileContent Error: " << vsShaderPath << std::endl;
		return -1;
	}

	ret = LoadFileContent(fsShaderPath, &fsCode);
	if (ret < 0)
	{
		std::cout << "fs shader LoadFileContent Error: " << fsShaderPath << std::endl;
		return -1;
	}

	//ram->vram, 从内存传到显存
	glShaderSource(vsShader, 1, (const char**)&vsCode, nullptr);//附加到着色器对象上
	glShaderSource(fsShader, 1, (const char**)&fsCode, nullptr);

	//进行GPU上编译
	glCompileShader(vsShader);//检查错误方法见补充
	CheckCompile(vsShader);

	glCompileShader(fsShader);
	CheckCompile(vsShader);

	//绑定程序program
	this->program = glCreateProgram();
	glAttachShader(program, vsShader);
	glAttachShader(program, fsShader);

	//链接
	glLinkProgram(program);//检查错误方法见补充

	//check glLinkProgram
	// 检查 shader 语法问题
	int success = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		memset(infoLog, 0, sizeof(infoLog) / sizeof(char));
		glGetProgramInfoLog(program, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
		return -1;
	}

    //在把着色器对象链接到程序对象以后，记得解绑定和删除着色器对象
	//1.解绑定
	glDetachShader(program, vsShader);
	glDetachShader(program, fsShader);

	//2.删除
	glDeleteShader(vsShader);
	glDeleteShader(fsShader);

	if (vsCode)
		delete[] vsCode;
	if (fsCode)
		delete[] fsCode;

	return this->program;
}

int GLRenderer::initTriangle()
{
	float vertices[] = {
		 0.5f,  0.5f, 0.0f,  // top right
		 0.5f, -0.5f, 0.0f,  // bottom right
		-0.5f, -0.5f, 0.0f,  // bottom left
		-0.5f,  0.5f, 0.0f   // top left 
	};
	unsigned int indices[] = {  // note that we start from 0!
		0, 1, 3,  // first Triangle
		1, 2, 3   // second Triangle
	};

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// 1. 绑定VAO
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	// 2. 把顶点数组复制到缓冲中供OpenGL使用
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//用glBufferData把索引复制到缓冲里
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	//从VBO中拿取一组pos通用属性组 对应的数据
	// 第一个参数指定我们要配置的顶点属性。我们在顶点着色器中使用layout(location = 0)定义了position顶点属性的位置值
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);

	return 0;
}

void GLRenderer::GetRendererObject(GLuint& vao, GLuint& vbo, GLuint& ebo)
{
	vao = this->VAO;
	vbo = this->VBO;
	ebo = this->EBO;
}