#include "glRenderer.h"
#include <stdio.h>
#include <iostream>
#include "Glm/glm.hpp"  // 数学库，计算矩阵
#include "Glm/ext.hpp"
#include "gpuProgram.h"
#include "objModel.h"
#include "misc.h"

struct Vertex
{
    float pos[3];
    float color[4] = { 1.0f,1.0f,1.0f,1.0f };  // 默认白色
};

struct MVPMatrix
{
    glm::mat4 model;
    glm::mat4 projection;
    glm::mat4 normalMatrix;
};


GLRenderer::GLRenderer(/* args */) :
    VAO(0), VBO(0), EBO(0),
    vsCode(nullptr), fsCode(nullptr),
    gpuProgram(nullptr)
{
}

GLRenderer::~GLRenderer()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    if (gpuProgram != nullptr)
    {
        delete gpuProgram;
        gpuProgram = nullptr;
    }

    if (objModel != nullptr)
    {
        delete objModel;
        objModel = nullptr;
    }

    if (mModelMvp != nullptr)
    {
        delete mModelMvp;
        mModelMvp = nullptr;
    }

	if (mFBO != nullptr)
	{
		delete mFBO;
        mFBO = nullptr;
	}

    if (mFullScreenQuad != nullptr)
    {
        delete mFullScreenQuad;
        mFullScreenQuad = nullptr;
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

    // init opengl var
    gpuProgram = new GPUProgram();
    objModel   = new ObjModel();
    mModelMvp   = new MVPMatrix();
    mFBO = new FBO();
    mFullScreenQuad = new FullScreenQuad();
    return 0;
}


int GLRenderer::InitTriangle()
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

void GLRenderer::SetTriangle_ShaderQualifiers(const GLuint& program)
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
    glUniformMatrix4fv(PLocation, 1, GL_FALSE, glm::value_ptr(projection)); // CPU -> copy -> GPU

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

float* CreatePerspective(float fov, float aspect, float zNear, float zFar)
{
	float* matrix = new float[16];
	float half = fov / 2.0f;
	float randiansOfHalf = (half / 180.0f) * 3.14f;
	float yscale = cosf(randiansOfHalf) / sinf(randiansOfHalf);
	float xscale = yscale / aspect;
	memset(matrix, 0, sizeof(float) * 16);
	matrix[0] = xscale;
	matrix[5] = yscale;
	matrix[10] = (zNear + zFar) / (zNear - zFar);
	matrix[11] = -1.0f;
	matrix[14] = (2.0f * zNear * zFar) / (zNear - zFar);
	return matrix;
}

int GLRenderer::InitModel()
{
    // 0.get Program
    if (nullptr == gpuProgram)
        return 0;
    gpuProgram->AttachShader(GL_VERTEX_SHADER, S_PATH("shader/sample.vs"));
    gpuProgram->AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/sample.fs"));
    gpuProgram->Link();
    GLuint program = gpuProgram->GetGPUProgram();
	GL_CHECK_ERROR;

    // 1.model
    objModel->InitModel(S_PATH("resource/model/Cube.obj"));

    // 2.传递参数到shader
    gpuProgram->DetectAttributes({"pos", "texcoord", "normal" });
    gpuProgram->DetectUniforms({ "M", "V", "P", "U_MainTexture", "U_Wood"});
    GL_CHECK_ERROR;

    // 3.根据图片创建纹理
    mMainTex = CreateTextureFromFile(S_PATH("resource/image/test.bmp"));
    mMulTex1 = CreateTextureFromFile(S_PATH("resource/image/wood.bmp"));

    // 4.初始化 mvp
    //mModelMvp->model = glm::translate(0.0f, 0.0f, -4.0f);
    //mModelMvp->projection = glm::perspective(45.0f, 800.0f / 600.0f, 0.1f, 1000.0f);
    //mModelMvp->normalMatrix = glm::inverseTranspose(mModelMvp->model);
    //mModelMvp->model = glm::translate<float>(0.0f, 0.0f, -2.0f) * glm::rotate<float>(-30.0f, 0.0f, 1.0f, 1.0f);
    //float* projection = CreatePerspective(50.0f, 800.0f / 600.0f, 0.1f, 1000.0f);

    // FBO
    mFBO->AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, 256, 256);
    mFBO->AttachDepthBuffer("depth", 256, 256);
    mFBO->Finish();


    // 5.opengl 环境设置
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    //glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return 0;
}

int GLRenderer::UpdateModel(float& angle)
{
    angle += 1.0f;
    if (angle > 360.0f)
        angle = 0;
    //glm::mat4 model = glm::translate(0.0f, 0.0f, -2.0f) * glm::rotate(angle, 0.0f, 1.0f, 0.0f);
    glm::mat4 model = glm::translate<float>(0.0f, 0.0f, -2.0f) * glm::rotate<float>(-30.0f, 0.0f, 1.0f, 1.0f);
    float* projection = CreatePerspective(50.0f, 800.0f / 600.0f, 0.1f, 1000.0f);
    //glm::mat4 normalMatrix = glm::inverseTranspose(model);

    mFBO->Bind();
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // FBO 纹理设置成白色
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    mFBO->UnBind();

	glClearColor(41.0f/255.0f, 71.0f/255.0f, 121.0f/255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // 编译命令
    glUseProgram(gpuProgram->GetGPUProgram());
    glUniformMatrix4fv(gpuProgram->GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(model));      // M model,模型视图移动，
    glUniformMatrix4fv(gpuProgram->GetQualfiterLoc("V"), 1, GL_FALSE, identity);                      // V visual 视口
    glUniformMatrix4fv(gpuProgram->GetQualfiterLoc("P"), 1, GL_FALSE, projection); // 投影
    //glUniformMatrix4fv(gpuProgram->GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
	GL_CHECK_ERROR;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mFBO->GetBuffer("color"));
	glUniform1i(gpuProgram->GetQualfiterLoc("U_MainTexture"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, mMulTex1);
    glUniform1i(gpuProgram->GetQualfiterLoc("U_Wood"), 1);
	GL_CHECK_ERROR;


    objModel->Bind(gpuProgram->GetQualfiterLoc("pos"), gpuProgram->GetQualfiterLoc("texcoord"), gpuProgram->GetQualfiterLoc("normal"));
    objModel->Draw();

    glUseProgram(0); // 重置

	GL_CHECK_ERROR;
    return 0;
}

int GLRenderer::InitFullScreenQuad()
{
	// 0.get Program
	if (nullptr == gpuProgram)
		return 0;
	gpuProgram->AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
	gpuProgram->AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/fullscreenQuad.fs"));
	gpuProgram->Link();
	GLuint program = gpuProgram->GetGPUProgram();
	GL_CHECK_ERROR;

	// 2.传递参数到shader
	gpuProgram->DetectAttributes({ "pos"});
	gpuProgram->DetectUniforms({"U_MainTexture"});
	GL_CHECK_ERROR;

    mFullScreenQuad->Init();
}

int GLRenderer::FullScreenQuadFun(HWND hwnd, HDC dc)
{
    // init
	// 0.get Program
    GPUProgram gpuProgModel;
    gpuProgModel.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/sample.vs"));
    gpuProgModel.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/sample.fs"));
    gpuProgModel.Link();
	GLuint progModel = gpuProgModel.GetGPUProgram();
	GL_CHECK_ERROR;

	// 1.model
	objModel->InitModel(S_PATH("resource/model/Cube.obj"));

	// 2.传递参数到shader
    gpuProgModel.DetectAttributes({ "pos", "texcoord", "normal" });
    gpuProgModel.DetectUniforms({ "M", "V", "P", "U_MainTexture", "U_Wood" });
	GL_CHECK_ERROR;

	// 3.根据图片创建纹理
	mMainTex = CreateTextureFromFile(S_PATH("resource/image/test.bmp"));
	mMulTex1 = CreateTextureFromFile(S_PATH("resource/image/wood.bmp"));


	// 4.FBO
	GPUProgram gpuProgQuad;
    gpuProgQuad.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    gpuProgQuad.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/fullscreenQuad.fs"));
    gpuProgQuad.Link();
	GLuint progQuad = gpuProgQuad.GetGPUProgram();
    gpuProgQuad.DetectAttributes({ "pos"});
    gpuProgQuad.DetectUniforms({"U_MainTexture"});

	mFBO->AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, 800, 600);
	mFBO->AttachDepthBuffer("depth", 800, 600);
	mFBO->Finish();

    mFullScreenQuad->Init();

	// 5.opengl 环境设置
	glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	glm::mat4 model = glm::translate<float>(0.0f, 0.0f, -2.0f) * glm::rotate<float>(-30.0f, 0.0f, 1.0f, 1.0f);
	float* projection = CreatePerspective(50.0f, 800.0f / 600.0f, 0.1f, 1000.0f);
	//glm::mat4 normalMatrix = glm::inverseTranspose(model);

	MSG msg;
	// 防止程序退出
	while (true)
{
		// Windows Message
		if (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
{
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// ----OpenGL start-----
	mFBO->Bind();
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // FBO 纹理设置成白色
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// 编译命令
		glUseProgram(gpuProgModel.GetGPUProgram());
		glUniformMatrix4fv(gpuProgModel.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(model));      // M model,模型视图移动，
		glUniformMatrix4fv(gpuProgModel.GetQualfiterLoc("V"), 1, GL_FALSE, identity);                      // V visual 视口
		glUniformMatrix4fv(gpuProgModel.GetQualfiterLoc("P"), 1, GL_FALSE, projection); // 投影
		//glUniformMatrix4fv(gpuProgram->GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
		GL_CHECK_ERROR;

	glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mMainTex);
		glUniform1i(gpuProgModel.GetQualfiterLoc("U_MainTexture"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, mMulTex1);
		glUniform1i(gpuProgModel.GetQualfiterLoc("U_Wood"), 1);

		objModel->Bind(gpuProgModel.GetQualfiterLoc("pos"), gpuProgModel.GetQualfiterLoc("texcoord"), gpuProgModel.GetQualfiterLoc("normal"));
		objModel->Draw();
		GL_CHECK_ERROR;

	glUseProgram(0); // 重置 
    mFBO->UnBind();
		glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(gpuProgQuad.GetGPUProgram());
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mFBO->GetBuffer("color"));
		glUniform1i(gpuProgQuad.GetQualfiterLoc("U_MainTexture"), 0);
        mFullScreenQuad->Draw(gpuProgQuad.GetQualfiterLoc("pos"));
        GL_CHECK_ERROR;

		glUseProgram(0); // 重置 
        glFinish();
		SwapBuffers(dc);
		// ----OpenGL end  -----
	}

    return 0;
}

void GLRenderer::GetRendererObject(GLuint& vao, GLuint& vbo, GLuint& ebo)
{
    vao = this->VAO;
    vbo = this->VBO;
    ebo = this->EBO;
}

GLuint GLRenderer::CreateTextureFromFile(const char* imagePath)
{
    unsigned char* imageData = nullptr;
    int ret = misc::LoadFileContent(imagePath, (char**)&imageData);
	if (ret < 0)
	{
		printf("LoadFileContent(%s) error\n", imagePath);
		return 0;
	}

    // decode bmp
    int width = 0, height = 0;
    unsigned char* pixelData = nullptr;
    int pixelDataSize = 0;
    GLenum srcFormat = GL_RGB;

    if (*((unsigned short*)imageData) == 0x4D42) // 判断前两个字节是否是 4D42，是的话就是bmp文件
    {
        pixelData = misc::DecodeBMPData(imageData, width, height);
    }
    else if (memcmp(imageData, "DDS ", 4) == 0)
    {
        pixelData = misc::DecodeDXT1Data(imageData, width, height, pixelDataSize);
        srcFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    }

    if (pixelData == nullptr)
    {
        printf("cannot decode %s\n", imagePath);
        delete imageData;
        return 0;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture); // 默认激活

    // 过滤条件
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (srcFormat == GL_RGB)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixelData);
    }
    else if (srcFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
    {
        glCompressedTexImage2D(GL_TEXTURE_2D, 0, srcFormat, width, height, 0, pixelDataSize, pixelData);
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    delete imageData;
    return texture;
}