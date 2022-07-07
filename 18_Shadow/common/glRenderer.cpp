#include "glRenderer.h"
#include <stdio.h>
#include <iostream>
#include "Glm/glm.hpp"  // 数学库，计算矩阵
#include "Glm/ext.hpp"
#include "SOIL.h"
#include "gpuProgram.h"
#include "objModel.h"
#include "misc.h"

struct Vertex
{
    float pos[3];
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // 默认白色
};

struct MVPMatrix
{
    glm::mat4 model;
    glm::mat4 projection;
    glm::mat4 normalMatrix;
};

GLRenderer::GLRenderer(/* args */) : VAO(0), VBO(0), EBO(0),
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
    objModel = new ObjModel();
    mModelMvp = new MVPMatrix();
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

    unsigned int indexes[] = {0, 1, 2}; // 连接顺序0-1-2

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

void GLRenderer::SetTriangle_ShaderQualifiers(const GLuint &program)
{
    // 单位矩阵
    float identity[] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1};

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
    glVertexAttribPointer(posLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(0)); // 每个点间隔Vertex大小,从0开始
    glEnableVertexAttribArray(colorLocation);
    glVertexAttribPointer(colorLocation, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(sizeof(float) * 3)); // 每个点间隔Vertex大小,起点是sizeof(float)*3即pos后面

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0); // 重置

    GL_CHECK_ERROR;
}

float *CreatePerspective(float fov, float aspect, float zNear, float zFar)
{
    float *matrix = new float[16];
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
    gpuProgram->DetectAttributes({"pos", "texcoord", "normal"});
    gpuProgram->DetectUniforms({"M", "V", "P", "U_MainTexture", "U_Wood"});
    GL_CHECK_ERROR;

    // 3.根据图片创建纹理
    mMainTex = CreateTextureFromFile(S_PATH("resource/image/test.bmp"));
    mMulTex1 = CreateTextureFromFile(S_PATH("resource/image/wood.bmp"));

    // 4.初始化 mvp
    // mModelMvp->model = glm::translate(0.0f, 0.0f, -4.0f);
    // mModelMvp->projection = glm::perspective(45.0f, 800.0f / 600.0f, 0.1f, 1000.0f);
    // mModelMvp->normalMatrix = glm::inverseTranspose(mModelMvp->model);
    // mModelMvp->model = glm::translate<float>(0.0f, 0.0f, -2.0f) * glm::rotate<float>(-30.0f, 0.0f, 1.0f, 1.0f);
    // float* projection = CreatePerspective(50.0f, 800.0f / 600.0f, 0.1f, 1000.0f);

    // FBO
    mFBO->AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, 256, 256);
    mFBO->AttachDepthBuffer("depth", 256, 256);
    mFBO->Finish();

    // 5.opengl 环境设置
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    // glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return 0;
}

int GLRenderer::UpdateModel(float &angle)
{
    angle += 1.0f;
    if (angle > 360.0f)
        angle = 0;
    // glm::mat4 model = glm::translate(0.0f, 0.0f, -2.0f) * glm::rotate(angle, 0.0f, 1.0f, 0.0f);
    glm::mat4 model = glm::translate<float>(0.0f, 0.0f, -2.0f) * glm::rotate<float>(-30.0f, 0.0f, 1.0f, 1.0f);
    float *projection = CreatePerspective(50.0f, 800.0f / 600.0f, 0.1f, 1000.0f);
    // glm::mat4 normalMatrix = glm::inverseTranspose(model);

    mFBO->Bind();
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // FBO 纹理设置成白色
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    mFBO->UnBind();

    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 编译命令
    glUseProgram(gpuProgram->GetGPUProgram());
    glUniformMatrix4fv(gpuProgram->GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(model)); // M model,模型视图移动，
    glUniformMatrix4fv(gpuProgram->GetQualfiterLoc("V"), 1, GL_FALSE, identity);              // V visual 视口
    glUniformMatrix4fv(gpuProgram->GetQualfiterLoc("P"), 1, GL_FALSE, projection);            // 投影
    // glUniformMatrix4fv(gpuProgram->GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
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
    gpuProgram->DetectAttributes({"pos"});
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
    gpuProgModel.DetectAttributes({"pos", "texcoord", "normal"});
    gpuProgModel.DetectUniforms({"M", "V", "P", "U_MainTexture", "U_Wood"});
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
    gpuProgQuad.DetectAttributes({"pos"});
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
    float *projection = CreatePerspective(50.0f, 800.0f / 600.0f, 0.1f, 1000.0f);
    // glm::mat4 normalMatrix = glm::inverseTranspose(model);

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
        glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 编译命令
        glUseProgram(gpuProgModel.GetGPUProgram());
        glUniformMatrix4fv(gpuProgModel.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(model)); // M model,模型视图移动，
        glUniformMatrix4fv(gpuProgModel.GetQualfiterLoc("V"), 1, GL_FALSE, identity);              // V visual 视口
        glUniformMatrix4fv(gpuProgModel.GetQualfiterLoc("P"), 1, GL_FALSE, projection);            // 投影
        // glUniformMatrix4fv(gpuProgram->GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
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
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // FBO 纹理设置成白色
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(gpuProgQuad.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mFBO->GetBuffer("color"));
        glUniform1i(gpuProgQuad.GetQualfiterLoc("U_MainTexture"), 0);
        // mFullScreenQuad->Draw(gpuProgQuad.GetQualfiterLoc("pos"));
        GL_CHECK_ERROR;

        glUseProgram(0); // 重置
        glFinish();
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

    return 0;
}

int GLRenderer::LightAmbientEnv(HWND hwnd, HDC dc)
{
    // init
    // 0.get Program
    GPUProgram gpuProgAmbient;
    gpuProgAmbient.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/ambient.vs"));
    gpuProgAmbient.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/ambient.fs"));
    gpuProgAmbient.Link();
    GL_CHECK_ERROR;

    // model
    objModel->InitModel(S_PATH("resource/model/Sphere.obj"));

    // 传递参数到shader
    gpuProgAmbient.DetectAttributes({"pos", "texcoord", "normal"});
    gpuProgAmbient.DetectUniforms({"M", "V", "P", "U_AmbientLightColor", "U_AmbientMaterial"});
    GL_CHECK_ERROR;

    // opengl 环境设置
    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    glm::mat4 model = glm::translate<float>(0.0f, 0.0f, -5.0f);
    glm::mat4 projection = glm::perspective(50.0f, 800.0f / 600.0f, 0.1f, 1000.0f);
    // glm::mat4 normalMatrix = glm::inverseTranspose(model);

    float ambientLightColor[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float ambientMaterial[] = {0.4f, 0.4f, 0.4f, 1.0f};

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
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 编译命令
        glUseProgram(gpuProgAmbient.GetGPUProgram());
        glUniformMatrix4fv(gpuProgAmbient.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(model)); // M model,模型视图移动，
        glUniformMatrix4fv(gpuProgAmbient.GetQualfiterLoc("V"), 1, GL_FALSE, identity);              // V visual 视口
        glUniformMatrix4fv(gpuProgAmbient.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));  // 投影
        glUniform4fv(gpuProgAmbient.GetQualfiterLoc("U_AmbientLightColor"), 1, ambientLightColor);
        glUniform4fv(gpuProgAmbient.GetQualfiterLoc("U_AmbientMaterial"), 1, ambientMaterial);
        GL_CHECK_ERROR;

        objModel->Bind(gpuProgAmbient.GetQualfiterLoc("pos"));
        objModel->Draw();
        GL_CHECK_ERROR;

        glUseProgram(0); // 重置
        glFinish();
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

    return 0;
}

int GLRenderer::LightDiffusePixelEnv(HWND hwnd, HDC dc)
{
    // init
    // 0.get Program
    GPUProgram gpuProgDiffuse;
    gpuProgDiffuse.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/diffuse_pixel.vs"));
    gpuProgDiffuse.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/diffuse_pixel.fs"));
    gpuProgDiffuse.Link();
    GL_CHECK_ERROR;

    // model
    objModel->InitModel(S_PATH("resource/model/Sphere.obj"));

    // 传递参数到shader
    gpuProgDiffuse.DetectAttributes({"pos", "texcoord", "normal"});
    gpuProgDiffuse.DetectUniforms({"M", "V", "P", "NM", "U_LightPos",
                                    "U_DiffuseLightColor", "U_DiffuseMaterial",
                                    "U_AmbientLightColor", "U_AmbientMaterial"});
    GL_CHECK_ERROR;

    // opengl 环境设置
    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    glm::mat4 model = glm::translate<float>(0.0f, 0.0f, -5.0f);
    glm::mat4 projection = glm::perspective(50.0f, 800.0f / 600.0f, 0.1f, 1000.0f);
    glm::mat4 normalMatrix = glm::inverseTranspose(model);

    float ambientLightColor[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float ambientMaterial[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float diffuseLightColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float diffuseMaterial[] = {0.8f, 0.8f, 0.8f, 1.0f};
    float lightPos[] = {1.0f, 1.0f, 0.0f};
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
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 编译命令
        glUseProgram(gpuProgDiffuse.GetGPUProgram());
        glUniformMatrix4fv(gpuProgDiffuse.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(model)); // M model,模型视图移动，
        glUniformMatrix4fv(gpuProgDiffuse.GetQualfiterLoc("V"), 1, GL_FALSE, identity);              // V visual 视口
        glUniformMatrix4fv(gpuProgDiffuse.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));  // 投影
        glUniformMatrix4fv(gpuProgDiffuse.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影

        // ambient
        glUniform4fv(gpuProgDiffuse.GetQualfiterLoc("U_AmbientLightColor"), 1, ambientLightColor);
        glUniform4fv(gpuProgDiffuse.GetQualfiterLoc("U_AmbientMaterial"), 1, ambientMaterial);

        // diffuse
        glUniform4fv(gpuProgDiffuse.GetQualfiterLoc("U_DiffuseLightColor"), 1, diffuseLightColor);
        glUniform4fv(gpuProgDiffuse.GetQualfiterLoc("U_DiffuseMaterial"), 1, diffuseMaterial);
        glUniform3fv(gpuProgDiffuse.GetQualfiterLoc("U_LightPos"), 1, lightPos);
        GL_CHECK_ERROR;

        objModel->Bind(gpuProgDiffuse.GetQualfiterLoc("pos"), gpuProgDiffuse.GetQualfiterLoc("normal"));
        objModel->Draw();
        GL_CHECK_ERROR;

        glUseProgram(0); // 重置
        glFinish();
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

    return 0;
}

int GLRenderer::LightSpecularPixelEnv(HWND hwnd, HDC dc, int viewW, int viewH)
{
    // init
    // 0.get Program
    GPUProgram gpuProgSpecular;
    gpuProgSpecular.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/light.vs"));
    gpuProgSpecular.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/light.fs"));
    gpuProgSpecular.Link();
    GL_CHECK_ERROR;

    // model
    objModel->InitModel(S_PATH("resource/model/Quad.obj"));

    // 传递参数到shader
    gpuProgSpecular.DetectAttributes({"pos", "texcoord", "normal"});
    gpuProgSpecular.DetectUniforms({"M", "V", "P", "NM", "U_LightPos",
                                    "U_DiffuseLightColor", "U_DiffuseMaterial",
                                    "U_AmbientLightColor", "U_AmbientMaterial",
                                    "U_SpecularLightColor", "U_SpecularMaterial",
                                    "U_EyePos", "U_SpotLightDirect", "U_CutOffAngle"});
    GL_CHECK_ERROR;

    // opengl 环境设置
    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    glm::mat4 model = glm::translate<float>(0.0f, -1.0f, -5.0f) * glm::rotate<float>(-90.0f, 1.0f, 0.0f, 0.0f) *
                      glm::scale(3.0f, 3.0f, 3.0f);
    const float WH = static_cast<float>(viewW) / static_cast<float>(viewH);
    glm::mat4 projection = glm::perspective(50.0f, WH, 0.1f, 1000.0f);
    glm::mat4 normalMatrix = glm::inverseTranspose(model);

    float ambientLightColor[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float ambientMaterial[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float diffuseLightColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float diffuseMaterial[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float specularLightColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float specularMaterial[] = {0.8f, 0.8f, 0.8f, 1.0f};
    float lightPos[] = {0.0f, 2.0f, -6.0f, 1.0f};
    float eyePos[] = {0.0f, 0.0f, 0.0f};
    float spotLightDirection[] = {0.0f, -1.0f, 0.0f, 128.0f};
    float spotLightCutOffAngle = 15.0; // degree
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
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 编译命令
        glUseProgram(gpuProgSpecular.GetGPUProgram());
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(model)); // M model,模型视图移动，
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("V"), 1, GL_FALSE, identity);              // V visual 视口
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));  // 投影
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影

        // ambient
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_AmbientLightColor"), 1, ambientLightColor);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_AmbientMaterial"), 1, ambientMaterial);

        // diffuse
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_DiffuseLightColor"), 1, diffuseLightColor);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_DiffuseMaterial"), 1, diffuseMaterial);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_LightPos"), 1, lightPos);
        GL_CHECK_ERROR;

        // specular
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_SpecularLightColor"), 1, specularLightColor);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_SpecularMaterial"), 1, specularMaterial);
        glUniform3fv(gpuProgSpecular.GetQualfiterLoc("U_EyePos"), 1, eyePos);
        GL_CHECK_ERROR;

        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_SpotLightDirect"), 1, spotLightDirection);
        glUniform1f(gpuProgSpecular.GetQualfiterLoc("U_CutOffAngle"), spotLightCutOffAngle);
        GL_CHECK_ERROR;

        objModel->Bind(gpuProgSpecular.GetQualfiterLoc("pos"), gpuProgSpecular.GetQualfiterLoc("normal"));
        objModel->Draw();
        GL_CHECK_ERROR;

        glUseProgram(0); // 重置
        glFinish();
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

    return 0;
}

int GLRenderer::AllLightSource(HWND hwnd, HDC dc, int viewW, int viewH)
{
    // init
    // 0.get Program
    GPUProgram gpuProgSpecular;
    gpuProgSpecular.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/light.vs"));
    gpuProgSpecular.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/light.fs"));
    gpuProgSpecular.Link();
    GL_CHECK_ERROR;

    // model
    objModel->InitModel(S_PATH("resource/model/Cube.obj"));

    // 传递参数到shader
    gpuProgSpecular.DetectAttributes({"pos", "texcoord", "normal"});
    gpuProgSpecular.DetectUniforms({"M", "V", "P", "NM", "U_LightPos",
                                    "U_DiffuseLightColor", "U_DiffuseMaterial",
                                    "U_AmbientLightColor", "U_AmbientMaterial",
                                    "U_SpecularLightColor", "U_SpecularMaterial",
                                    "U_EyePos", "U_SpotLightDirect", "U_CutOffAngle"});
    GL_CHECK_ERROR;

    glm::mat4 modelA = glm::translate<float>(-2.0f, 0.0f, -6.0f) * glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    glm::mat4 modelB = glm::translate<float>( 2.0f, 0.0f, -6.0f) * glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    glm::mat4 modelC = glm::translate<float>( 6.0f, 0.0f, -6.0f) * glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    const float WH = static_cast<float>(viewW) / static_cast<float>(viewH);
    glm::mat4 projection = glm::perspective(50.0f, WH, 0.1f, 1000.0f);
    glm::mat4 normalMatrix = glm::inverseTranspose(modelA);
    glm::mat4 normalMatrixB = glm::inverseTranspose(modelB);
    glm::mat4 normalMatrixC = glm::inverseTranspose(modelC);

    glm::mat4 viewMatrix1 = glm::lookAt(glm::vec3(-0.5, 1.5f, -3.0f),
                                        glm::vec3(-2.0f, 0.0f, -6.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 viewMatrix2 = glm::lookAt(glm::vec3(2.0, 1.5f, -3.0f),
                                        glm::vec3(2.0f, 0.0f, -6.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 viewMatrix3 = glm::lookAt(glm::vec3(6, 1.5f, -3.0f),
                                        glm::vec3(6.0f, 0.0f, -6.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f));


    float ambientLightColor[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float ambientMaterial[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float diffuseLightColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float diffuseMaterial[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float specularLightColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float specularMaterial[] = {0.8f, 0.8f, 0.8f, 1.0f};
    float lightPos[] = {0.0f, 1.5f, 0.0f, 0.0f};
    float eyePos[] = {0.0f, 0.0f, 0.0f};
    float spotLightDirection[] = {0.0f, -1.0f, 0.0f, 128.0f};
    float spotLightCutOffAngle = 0.0; // degree

    // full screen shader
    GPUProgram fsqProgram;
    fsqProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    fsqProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/fullscreenQuad.fs"));
    fsqProgram.Link();
    GL_CHECK_ERROR;
    fsqProgram.DetectAttributes({"pos", "texcoord"});
    fsqProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    FullScreenQuad fsq;
    fsq.Init();

    FBO fboDirectionLight, fboPintLight, fboSpotLight;
    fboDirectionLight.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboDirectionLight.AttachDepthBuffer("depth", viewW, viewH);
    fboDirectionLight.Finish();

    fboPintLight.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboPintLight.AttachDepthBuffer("depth", viewW, viewH);
    fboPintLight.Finish();

    fboSpotLight.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboSpotLight.AttachDepthBuffer("depth", viewW, viewH);
    fboSpotLight.Finish();
    // FBO fbo;
    // fbo.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    // fbo.AttachDepthBuffer("depth", viewW, viewH);
    // fbo.Finish();
    GL_CHECK_ERROR;

    // opengl 环境设置
    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    MSG msg;
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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
        // 编译命令
        glUseProgram(gpuProgSpecular.GetGPUProgram());
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("V"), 1, GL_FALSE, glm::value_ptr(viewMatrix1));    // V visual 视口
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影

        // ambient
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_AmbientLightColor"), 1, ambientLightColor);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_AmbientMaterial"), 1, ambientMaterial);

        // diffuse
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_DiffuseLightColor"), 1, diffuseLightColor);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_DiffuseMaterial"), 1, diffuseMaterial);
        GL_CHECK_ERROR;

        // specular
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_SpecularLightColor"), 1, specularLightColor);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_SpecularMaterial"), 1, specularMaterial);
        glUniform3fv(gpuProgSpecular.GetQualfiterLoc("U_EyePos"), 1, eyePos);
        GL_CHECK_ERROR;

        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_SpotLightDirect"), 1, spotLightDirection);
        GL_CHECK_ERROR;

        // modelA, 方向光
        lightPos[0] = 0.0f;
        lightPos[1] = 1.5f;
        lightPos[2] = 0.0f;
        lightPos[3] = 0.0f;
        spotLightCutOffAngle = 0.0f;
        glUniform1f(gpuProgSpecular.GetQualfiterLoc("U_CutOffAngle"), spotLightCutOffAngle);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_LightPos"), 1, lightPos);
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(modelA)); // M model,模型视图移动，
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影
        GL_CHECK_ERROR;
        fboDirectionLight.Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        objModel->Bind(gpuProgSpecular.GetQualfiterLoc("pos"), gpuProgSpecular.GetQualfiterLoc("normal"));
        objModel->Draw();
        fboDirectionLight.UnBind();
        GL_CHECK_ERROR;


        // modelB, 点光源
        lightPos[0] = 2.0f;
        lightPos[1] = 3.0f;
        lightPos[2] = -6.0f;
        lightPos[3] = 1.0f;
        spotLightCutOffAngle = 0.0f;
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("V"), 1, GL_FALSE, glm::value_ptr(viewMatrix2));    // V visual 视口
        glUniform1f(gpuProgSpecular.GetQualfiterLoc("U_CutOffAngle"), spotLightCutOffAngle);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_LightPos"), 1, lightPos);
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(modelB)); // M model,模型视图移动，
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrixB));  // 投影
        fboPintLight.Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        objModel->Bind(gpuProgSpecular.GetQualfiterLoc("pos"), gpuProgSpecular.GetQualfiterLoc("normal"));
        objModel->Draw();
        fboPintLight.UnBind();
        GL_CHECK_ERROR;

        // modelC, 聚光灯
        lightPos[0] = 6.0f;
        lightPos[1] = 3.0f;
        lightPos[2] = -6.0f;
        lightPos[3] = 1.0f;
        spotLightCutOffAngle = 15.0f;
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("V"), 1, GL_FALSE, glm::value_ptr(viewMatrix3));    // V visual 视口
        glUniform1f(gpuProgSpecular.GetQualfiterLoc("U_CutOffAngle"), spotLightCutOffAngle);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_LightPos"), 1, lightPos);
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(modelC)); // M model,模型视图移动，
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrixC));  // 投影
        fboSpotLight.Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        objModel->Bind(gpuProgSpecular.GetQualfiterLoc("pos"), gpuProgSpecular.GetQualfiterLoc("normal"));
        objModel->Draw();
        fboSpotLight.UnBind();
        glFinish();
        GL_CHECK_ERROR;

        // FBO Draw
        // fbo.UnBind();
        glUseProgram(0); // 重置

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(fsqProgram.GetGPUProgram());
        glUniform1i(fsqProgram.GetQualfiterLoc("U_MainTexture"), 0);

        glBindTexture(GL_TEXTURE_2D, fboDirectionLight.GetBuffer("color"));
        // fsq.Draw(fsqProgram.GetQualfiterLoc("pos"), fsqProgram.GetQualfiterLoc("texcoord"));
        fsq.DrawToQuarter(fsqProgram.GetQualfiterLoc("pos"), fsqProgram.GetQualfiterLoc("texcoord"), 0);

        glBindTexture(GL_TEXTURE_2D, fboPintLight.GetBuffer("color"));
        fsq.DrawToQuarter(fsqProgram.GetQualfiterLoc("pos"), fsqProgram.GetQualfiterLoc("texcoord"), 1);

        glBindTexture(GL_TEXTURE_2D, fboSpotLight.GetBuffer("color"));
        fsq.DrawToQuarter(fsqProgram.GetQualfiterLoc("pos"), fsqProgram.GetQualfiterLoc("texcoord"), 2);
        glBindTexture(GL_TEXTURE_2D, 0);

        glUseProgram(0); // 重置
        glFinish();
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

    return 0;
}

int GLRenderer::UseXRay(HWND hwnd, HDC dc, int viewW, int viewH)
{
    // init
    // 0.get Program
    GPUProgram gpuProgSpecular;
    gpuProgSpecular.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/xRay.vs"));
    gpuProgSpecular.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/xRay.fs"));
    gpuProgSpecular.Link();
    GL_CHECK_ERROR;

    // model
    objModel->InitModel(S_PATH("resource/model/Sphere.obj"));

    // 传递参数到shader
    gpuProgSpecular.DetectAttributes({"pos", "texcoord", "normal"});
    gpuProgSpecular.DetectUniforms({"M", "V", "P", "NM", "U_LightPos",
                                    "U_EyePos"});
    GL_CHECK_ERROR;

    glm::mat4 modelA = glm::translate<float>(-2.0f, 0.0f, -6.0f) * glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    const float WH = static_cast<float>(viewW) / static_cast<float>(viewH);
    glm::mat4 projection = glm::perspective(50.0f, WH, 0.1f, 1000.0f);
    glm::mat4 normalMatrix = glm::inverseTranspose(modelA);

    glm::mat4 viewMatrix1 = glm::lookAt(glm::vec3(-0.5, 1.5f, -3.0f),
                                        glm::vec3(-2.0f, 0.0f, -6.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f));

    float lightPos[] = {0.0f, 1.5f, 0.0f, 0.0f};
    float eyePos[] = {-0.5, 1.5f, -3.0f};

    // opengl 环境设置
    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // 开启 alpha 混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    MSG msg;
    glClearColor(0.1f, 0.4f, 0.7f, 1.0f);
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
        // 编译命令
        glUseProgram(gpuProgSpecular.GetGPUProgram());
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("V"), 1, GL_FALSE, glm::value_ptr(viewMatrix1));    // V visual 视口
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影

        glUniform3fv(gpuProgSpecular.GetQualfiterLoc("U_EyePos"), 1, eyePos);
        GL_CHECK_ERROR;

        // modelA, 方向光
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_LightPos"), 1, lightPos);
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(modelA)); // M model,模型视图移动，
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影
        GL_CHECK_ERROR;
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        objModel->Bind(gpuProgSpecular.GetQualfiterLoc("pos"), gpuProgSpecular.GetQualfiterLoc("normal"));
        objModel->Draw();
        GL_CHECK_ERROR;

        glUseProgram(0); // 重置
        glFinish();
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

    glDisable(GL_BLEND);
    return 0;
}

int GLRenderer::Erosion_Dilation_Guassian(HWND hwnd, HDC dc, int viewW, int viewH)
{
    // init
    // 0.get Program
    GPUProgram gpuProgSpecular;
    gpuProgSpecular.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/xRay.vs"));
    gpuProgSpecular.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/xRay.fs"));
    gpuProgSpecular.Link();
    GL_CHECK_ERROR;

    GPUProgram originProgram;
    originProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    originProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/fullscreenQuad.fs"));
    originProgram.Link();
    GL_CHECK_ERROR;
    originProgram.DetectAttributes({"pos", "texcoord"});
    originProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram erosionProgram;
    erosionProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    erosionProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/erosion.fs"));
    erosionProgram.Link();
    GL_CHECK_ERROR;
    erosionProgram.DetectAttributes({"pos", "texcoord"});
    erosionProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram dilationProgram;
    dilationProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    dilationProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/dilation.fs"));
    dilationProgram.Link();
    GL_CHECK_ERROR;
    dilationProgram.DetectAttributes({"pos", "texcoord"});
    dilationProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram gaussianProgram;
    gaussianProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    gaussianProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/gaussian.fs"));
    gaussianProgram.Link();
    GL_CHECK_ERROR;
    gaussianProgram.DetectAttributes({"pos", "texcoord"});
    gaussianProgram.DetectUniforms({"U_MainTexture"});
    GLuint blurTex = CreateTextureFromFile(S_PATH("resource/image/wood.bmp"));
    GL_CHECK_ERROR;

    FullScreenQuad fsq;
    fsq.Init();

    FBO fbo;
    fbo.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fbo.AttachDepthBuffer("depth", viewW, viewH);
    fbo.Finish();

    // model
    objModel->InitModel(S_PATH("resource/model/Sphere.obj"));

    // 传递参数到shader
    gpuProgSpecular.DetectAttributes({"pos", "texcoord", "normal"});
    gpuProgSpecular.DetectUniforms({"M", "V", "P", "NM", "U_LightPos",
                                    "U_EyePos"});
    GL_CHECK_ERROR;

    glm::mat4 modelA = glm::translate<float>(-2.0f, 0.0f, -6.0f) * glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    const float WH = static_cast<float>(viewW) / static_cast<float>(viewH);
    glm::mat4 projection = glm::perspective(50.0f, WH, 0.1f, 1000.0f);
    glm::mat4 normalMatrix = glm::inverseTranspose(modelA);

    glm::mat4 viewMatrix1 = glm::lookAt(glm::vec3(-0.5, 1.5f, -3.0f),
                                        glm::vec3(-2.0f, 0.0f, -6.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f));

    float lightPos[] = {0.0f, 1.5f, 0.0f, 0.0f};
    float eyePos[] = {-0.5, 1.5f, -3.0f};

    // opengl 环境设置
    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // 开启 alpha 混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    MSG msg;
    glClearColor(0.1f, 0.4f, 0.7f, 1.0f);
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
        // 编译命令
        glUseProgram(gpuProgSpecular.GetGPUProgram());
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("V"), 1, GL_FALSE, glm::value_ptr(viewMatrix1));    // V visual 视口
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影

        glUniform3fv(gpuProgSpecular.GetQualfiterLoc("U_EyePos"), 1, eyePos);
        GL_CHECK_ERROR;

        // modelA, 方向光
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_LightPos"), 1, lightPos);
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(modelA)); // M model,模型视图移动，
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影
        GL_CHECK_ERROR;

        fbo.Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        objModel->Bind(gpuProgSpecular.GetQualfiterLoc("pos"), gpuProgSpecular.GetQualfiterLoc("normal"));
        objModel->Draw();
        GL_CHECK_ERROR;
        glFlush();
        fbo.UnBind();

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(originProgram.GetGPUProgram());
        glUniform1i(originProgram.GetQualfiterLoc("U_MainTexture"), 0);
        glBindTexture(GL_TEXTURE_2D, fbo.GetBuffer("color"));
        fsq.DrawToQuarter(originProgram.GetQualfiterLoc("pos"), originProgram.GetQualfiterLoc("texcoord"), 0);


        glUseProgram(erosionProgram.GetGPUProgram());
        glUniform1i(erosionProgram.GetQualfiterLoc("U_MainTexture"), 0);
        glBindTexture(GL_TEXTURE_2D, fbo.GetBuffer("color"));
        fsq.DrawToQuarter(erosionProgram.GetQualfiterLoc("pos"), erosionProgram.GetQualfiterLoc("texcoord"), 1);


        glUseProgram(dilationProgram.GetGPUProgram());
        glUniform1i(dilationProgram.GetQualfiterLoc("U_MainTexture"), 0);
        glBindTexture(GL_TEXTURE_2D, fbo.GetBuffer("color"));
        fsq.DrawToQuarter(dilationProgram.GetQualfiterLoc("pos"), dilationProgram.GetQualfiterLoc("texcoord"), 2);

        glUseProgram(gaussianProgram.GetGPUProgram());
        glUniform1i(gaussianProgram.GetQualfiterLoc("U_MainTexture"), 0);
        glBindTexture(GL_TEXTURE_2D, blurTex);
        fsq.DrawToQuarter(gaussianProgram.GetQualfiterLoc("pos"), dilationProgram.GetQualfiterLoc("texcoord"), 3);

        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0); // 重置
        glFinish();
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

    glDisable(GL_BLEND);
    return 0;
}

int GLRenderer::Erosion_Dilation_MultiGuassian(HWND hwnd, HDC dc, int viewW, int viewH)
{
    // init
    // 0.get Program
    GPUProgram gpuProgSpecular;
    gpuProgSpecular.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/light.vs"));
    gpuProgSpecular.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/light.fs"));
    gpuProgSpecular.Link();
    GL_CHECK_ERROR;

    GPUProgram originProgram;
    originProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    originProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/fullscreenQuad.fs"));
    originProgram.Link();
    originProgram.DetectAttributes({"pos", "texcoord"});
    originProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram gaussianProgram;
    gaussianProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    gaussianProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/gaussian.fs"));
    gaussianProgram.Link();
    GL_CHECK_ERROR;
    gaussianProgram.DetectAttributes({"pos", "texcoord"});
    gaussianProgram.DetectUniforms({"U_MainTexture"});
    GLuint blurTex = CreateTextureFromFile(S_PATH("resource/image/wood.bmp"));
    GL_CHECK_ERROR;

    FullScreenQuad fsq;
    fsq.Init();

    FBO fboDirectionLight;
    fboDirectionLight.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboDirectionLight.AttachDepthBuffer("depth", viewW, viewH);
    fboDirectionLight.Finish();

    FBO fboGaussian1;
    fboGaussian1.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboGaussian1.AttachDepthBuffer("depth", viewW, viewH);
    fboGaussian1.Finish();

    FBO fboGaussian2;
    fboGaussian2.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboGaussian2.AttachDepthBuffer("depth", viewW, viewH);
    fboGaussian2.Finish();

    FBO fboGaussian3;
    fboGaussian3.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboGaussian3.AttachDepthBuffer("depth", viewW, viewH);
    fboGaussian3.Finish();

    // model
    objModel->InitModel(S_PATH("resource/model/Cube.obj"));

    // 传递参数到shader
    gpuProgSpecular.DetectAttributes({"pos", "texcoord", "normal"});
    gpuProgSpecular.DetectUniforms({"M", "V", "P", "NM", "U_LightPos",
                                    "U_DiffuseLightColor", "U_DiffuseMaterial",
                                    "U_AmbientLightColor", "U_AmbientMaterial",
                                    "U_SpecularLightColor", "U_SpecularMaterial",
                                    "U_EyePos", "U_SpotLightDirect", "U_CutOffAngle"});
    GL_CHECK_ERROR;

    glm::mat4 modelA = glm::translate<float>(-2.0f, 0.0f, -6.0f) * glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    const float WH = static_cast<float>(viewW) / static_cast<float>(viewH);
    glm::mat4 projection = glm::perspective(50.0f, WH, 0.1f, 1000.0f);
    glm::mat4 normalMatrix = glm::inverseTranspose(modelA);
    glm::mat4 viewMatrix1 = glm::lookAt(glm::vec3(-0.5, 1.5f, -3.0f),
                                        glm::vec3(-2.0f, 0.0f, -6.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f));
    float ambientLightColor[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float ambientMaterial[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float diffuseLightColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float diffuseMaterial[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float specularLightColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float specularMaterial[] = {0.8f, 0.8f, 0.8f, 1.0f};
    float lightPos[] = {0.0f, 1.5f, 0.0f, 0.0f};
    float eyePos[] = {0.0f, 0.0f, 0.0f};
    float spotLightDirection[] = {0.0f, -1.0f, 0.0f, 128.0f};
    float spotLightCutOffAngle = 0.0; // degree

    // opengl 环境设置
    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // 开启 alpha 混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    MSG msg;
    glClearColor(0.1f, 0.4f, 0.7f, 1.0f);
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
        // 编译命令
        glUseProgram(gpuProgSpecular.GetGPUProgram());
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("V"), 1, GL_FALSE, glm::value_ptr(viewMatrix1));    // V visual 视口
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影

        // ambient
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_AmbientLightColor"), 1, ambientLightColor);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_AmbientMaterial"), 1, ambientMaterial);

        // diffuse
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_DiffuseLightColor"), 1, diffuseLightColor);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_DiffuseMaterial"), 1, diffuseMaterial);
        GL_CHECK_ERROR;

        // specular
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_SpecularLightColor"), 1, specularLightColor);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_SpecularMaterial"), 1, specularMaterial);
        glUniform3fv(gpuProgSpecular.GetQualfiterLoc("U_EyePos"), 1, eyePos);
        GL_CHECK_ERROR;

        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_SpotLightDirect"), 1, spotLightDirection);
        GL_CHECK_ERROR;

        // 聚光
        lightPos[0] = -1.8f;
        lightPos[1] = 3.0f;
        lightPos[2] = -5.7f;
        lightPos[3] = 1.0f;
        spotLightCutOffAngle = 15.0f;
        glUniform1f(gpuProgSpecular.GetQualfiterLoc("U_CutOffAngle"), spotLightCutOffAngle);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_LightPos"), 1, lightPos);
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(modelA)); // M model,模型视图移动，
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影
        GL_CHECK_ERROR;
        fboDirectionLight.Bind();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        objModel->Bind(gpuProgSpecular.GetQualfiterLoc("pos"), gpuProgSpecular.GetQualfiterLoc("normal"));
        objModel->Draw();
        fboDirectionLight.UnBind();
        GL_CHECK_ERROR;
        glUseProgram(0); // 重置

        // no blur
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(originProgram.GetGPUProgram());
        glUniform1i(originProgram.GetQualfiterLoc("U_MainTexture"), 0);
        glBindTexture(GL_TEXTURE_2D, fboDirectionLight.GetBuffer("color"));
        fsq.DrawToQuarter(originProgram.GetQualfiterLoc("pos"), originProgram.GetQualfiterLoc("texcoord"), 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0); // 重置

        // blur x1
        fboGaussian1.Bind();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindTexture(GL_TEXTURE_2D, fboDirectionLight.GetBuffer("color"));
        glUseProgram(gaussianProgram.GetGPUProgram());
        fsq.DrawToQuarter(gaussianProgram.GetQualfiterLoc("pos"), gaussianProgram.GetQualfiterLoc("texcoord"), 5);
        fboGaussian1.UnBind();

        // blur x2
        fboGaussian2.Bind();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(gaussianProgram.GetGPUProgram());
        glBindTexture(GL_TEXTURE_2D, fboGaussian1.GetBuffer("color"));
        fsq.DrawToQuarter(gaussianProgram.GetQualfiterLoc("pos"), gaussianProgram.GetQualfiterLoc("texcoord"), 5);
        fboGaussian2.UnBind();

        // blur x3
        fboGaussian3.Bind();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(gaussianProgram.GetGPUProgram());
        glBindTexture(GL_TEXTURE_2D, fboGaussian2.GetBuffer("color"));
        fsq.DrawToQuarter(gaussianProgram.GetQualfiterLoc("pos"), gaussianProgram.GetQualfiterLoc("texcoord"), 5);
        fboGaussian3.UnBind();

        glBindTexture(GL_TEXTURE_2D, fboGaussian1.GetBuffer("color"));
        fsq.DrawToQuarter(gaussianProgram.GetQualfiterLoc("pos"), gaussianProgram.GetQualfiterLoc("texcoord"), 1);

        glBindTexture(GL_TEXTURE_2D, fboGaussian2.GetBuffer("color"));
        fsq.DrawToQuarter(gaussianProgram.GetQualfiterLoc("pos"), gaussianProgram.GetQualfiterLoc("texcoord"), 2);

        glBindTexture(GL_TEXTURE_2D, fboGaussian3.GetBuffer("color"));
        fsq.DrawToQuarter(gaussianProgram.GetQualfiterLoc("pos"), gaussianProgram.GetQualfiterLoc("texcoord"), 3);

        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0); // 重置
        glFinish();
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

    glDisable(GL_BLEND);
    return 0;
}

int GLRenderer::MultiGuassianSimplified(HWND hwnd, HDC dc, int viewW, int viewH)
{
    // init
    // 0.get Program
    GPUProgram gpuProgSpecular;
    gpuProgSpecular.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/light.vs"));
    gpuProgSpecular.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/light.fs"));
    gpuProgSpecular.Link();
    GL_CHECK_ERROR;

    GPUProgram originProgram;
    originProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    originProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/fullscreenQuad.fs"));
    originProgram.Link();
    originProgram.DetectAttributes({"pos", "texcoord"});
    originProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram verticalGaussProgram;
    verticalGaussProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    verticalGaussProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/gaussianVertical.fs"));
    verticalGaussProgram.Link();
    verticalGaussProgram.DetectAttributes({"pos", "texcoord"});
    verticalGaussProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram horizontalGaussProgram;
    horizontalGaussProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    horizontalGaussProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/gaussianHorizontal.fs"));
    horizontalGaussProgram.Link();
    horizontalGaussProgram.DetectAttributes({"pos", "texcoord"});
    horizontalGaussProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram gaussianProgram;
    gaussianProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    gaussianProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/gaussian.fs"));
    gaussianProgram.Link();
    GL_CHECK_ERROR;
    gaussianProgram.DetectAttributes({"pos", "texcoord"});
    gaussianProgram.DetectUniforms({"U_MainTexture"});
    GLuint blurTex = CreateTextureFromFile(S_PATH("resource/image/wood.bmp"));
    GL_CHECK_ERROR;

    FullScreenQuad fsq;
    fsq.Init();

    FBO fboDirectionLight;
    fboDirectionLight.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboDirectionLight.AttachDepthBuffer("depth", viewW, viewH);
    fboDirectionLight.Finish();

    FBO fboGaussian1;
    fboGaussian1.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboGaussian1.AttachDepthBuffer("depth", viewW, viewH);
    fboGaussian1.Finish();

    FBO fboGaussian2;
    fboGaussian2.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboGaussian2.AttachDepthBuffer("depth", viewW, viewH);
    fboGaussian2.Finish();

    FBO fboGaussian3;
    fboGaussian3.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboGaussian3.AttachDepthBuffer("depth", viewW, viewH);
    fboGaussian3.Finish();

    // model
    objModel->InitModel(S_PATH("resource/model/Cube.obj"));

    // 传递参数到shader
    gpuProgSpecular.DetectAttributes({"pos", "texcoord", "normal"});
    gpuProgSpecular.DetectUniforms({"M", "V", "P", "NM", "U_LightPos",
                                    "U_DiffuseLightColor", "U_DiffuseMaterial",
                                    "U_AmbientLightColor", "U_AmbientMaterial",
                                    "U_SpecularLightColor", "U_SpecularMaterial",
                                    "U_EyePos", "U_SpotLightDirect", "U_CutOffAngle"});
    GL_CHECK_ERROR;

    glm::mat4 modelA = glm::translate<float>(-2.0f, 0.0f, -6.0f) * glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    const float WH = static_cast<float>(viewW) / static_cast<float>(viewH);
    glm::mat4 projection = glm::perspective(50.0f, WH, 0.1f, 1000.0f);
    glm::mat4 normalMatrix = glm::inverseTranspose(modelA);
    glm::mat4 viewMatrix1 = glm::lookAt(glm::vec3(-0.5, 1.5f, -3.0f),
                                        glm::vec3(-2.0f, 0.0f, -6.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f));
    float ambientLightColor[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float ambientMaterial[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float diffuseLightColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float diffuseMaterial[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float specularLightColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float specularMaterial[] = {0.8f, 0.8f, 0.8f, 1.0f};
    float lightPos[] = {0.0f, 1.5f, 0.0f, 0.0f};
    float eyePos[] = {0.0f, 0.0f, 0.0f};
    float spotLightDirection[] = {0.0f, -1.0f, 0.0f, 128.0f};
    float spotLightCutOffAngle = 0.0; // degree

    // opengl 环境设置
    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // 开启 alpha 混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    MSG msg;
    glClearColor(0.1f, 0.4f, 0.7f, 1.0f);
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
        // 编译命令
        glUseProgram(gpuProgSpecular.GetGPUProgram());
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("V"), 1, GL_FALSE, glm::value_ptr(viewMatrix1));    // V visual 视口
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影

        // ambient
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_AmbientLightColor"), 1, ambientLightColor);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_AmbientMaterial"), 1, ambientMaterial);

        // diffuse
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_DiffuseLightColor"), 1, diffuseLightColor);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_DiffuseMaterial"), 1, diffuseMaterial);
        GL_CHECK_ERROR;

        // specular
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_SpecularLightColor"), 1, specularLightColor);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_SpecularMaterial"), 1, specularMaterial);
        glUniform3fv(gpuProgSpecular.GetQualfiterLoc("U_EyePos"), 1, eyePos);
        GL_CHECK_ERROR;

        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_SpotLightDirect"), 1, spotLightDirection);
        GL_CHECK_ERROR;

        // 聚光
        lightPos[0] = -1.8f;
        lightPos[1] = 3.0f;
        lightPos[2] = -5.7f;
        lightPos[3] = 1.0f;
        spotLightCutOffAngle = 15.0f;
        glUniform1f(gpuProgSpecular.GetQualfiterLoc("U_CutOffAngle"), spotLightCutOffAngle);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_LightPos"), 1, lightPos);
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(modelA)); // M model,模型视图移动，
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影
        GL_CHECK_ERROR;
        fboDirectionLight.Bind();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        objModel->Bind(gpuProgSpecular.GetQualfiterLoc("pos"), gpuProgSpecular.GetQualfiterLoc("normal"));
        objModel->Draw();
        fboDirectionLight.UnBind();
        GL_CHECK_ERROR;
        glUseProgram(0); // 重置

        // no blur
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(originProgram.GetGPUProgram());
        glUniform1i(originProgram.GetQualfiterLoc("U_MainTexture"), 0);
        glBindTexture(GL_TEXTURE_2D, fboDirectionLight.GetBuffer("color"));
        fsq.DrawToQuarter(originProgram.GetQualfiterLoc("pos"), originProgram.GetQualfiterLoc("texcoord"), 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0); // 重置

        // blur fbo
        fboGaussian1.Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(originProgram.GetGPUProgram());
        glBindTexture(GL_TEXTURE_2D, fboDirectionLight.GetBuffer("color"));
        glUniform1i(originProgram.GetQualfiterLoc("U_MainTexture"), 0);
        fsq.DrawToQuarter(originProgram.GetQualfiterLoc("pos"), originProgram.GetQualfiterLoc("texcoord"), 5);
        GL_CHECK_ERROR;
        fboGaussian1.UnBind();
        
        // blur fbo
        fboGaussian2.Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(verticalGaussProgram.GetGPUProgram());
        glBindTexture(GL_TEXTURE_2D, fboDirectionLight.GetBuffer("color"));
        glUniform1i(verticalGaussProgram.GetQualfiterLoc("U_MainTexture"), 0);
        fsq.DrawToQuarter(verticalGaussProgram.GetQualfiterLoc("pos"), verticalGaussProgram.GetQualfiterLoc("texcoord"), 5);
        GL_CHECK_ERROR;
        fboGaussian2.UnBind();
        


        glUseProgram(gaussianProgram.GetGPUProgram());
        glBindTexture(GL_TEXTURE_2D, fboGaussian1.GetBuffer("color"));
        fsq.DrawToQuarter(gaussianProgram.GetQualfiterLoc("pos"), gaussianProgram.GetQualfiterLoc("texcoord"), 1);

        glUseProgram(horizontalGaussProgram.GetGPUProgram());
        glBindTexture(GL_TEXTURE_2D, fboGaussian2.GetBuffer("color"));
        fsq.DrawToQuarter(horizontalGaussProgram.GetQualfiterLoc("pos"), horizontalGaussProgram.GetQualfiterLoc("texcoord"), 2);

        // glUseProgram(verticalGaussProgram.GetGPUProgram());
        // glBindTexture(GL_TEXTURE_2D, fboGaussian1.GetBuffer("color"));
        // fsq.DrawToQuarter(verticalGaussProgram.GetQualfiterLoc("pos"), verticalGaussProgram.GetQualfiterLoc("texcoord"), 3);
        // GL_CHECK_ERROR;



        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0); // 重置
        glFinish();
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

    glDisable(GL_BLEND);
    return 0;
}

int GLRenderer::UseHDR(HWND hwnd, HDC dc, int viewW, int viewH)
{
    // init
    // 0.get Program
    GPUProgram gpuProgSpecular;
    gpuProgSpecular.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/hdrLight.vs"));
    gpuProgSpecular.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/hdrLight.fs"));
    gpuProgSpecular.Link();
    GL_CHECK_ERROR;

    GPUProgram originProgram;
    originProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    originProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/fullscreenQuad.fs"));
    originProgram.Link();
    originProgram.DetectAttributes({"pos", "texcoord"});
    originProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram hdrProgram;
    hdrProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    hdrProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/hdrRenderer.fs"));
    hdrProgram.Link();
    hdrProgram.DetectAttributes({"pos", "texcoord"});
    hdrProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram verticalGaussProgram;
    verticalGaussProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    verticalGaussProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/gaussianVertical.fs"));
    verticalGaussProgram.Link();
    verticalGaussProgram.DetectAttributes({"pos", "texcoord"});
    verticalGaussProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram horizontalGaussProgram;
    horizontalGaussProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    horizontalGaussProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/gaussianHorizontal.fs"));
    horizontalGaussProgram.Link();
    horizontalGaussProgram.DetectAttributes({"pos", "texcoord"});
    horizontalGaussProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram gaussianProgram;
    gaussianProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    gaussianProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/gaussian.fs"));
    gaussianProgram.Link();
    GL_CHECK_ERROR;
    gaussianProgram.DetectAttributes({"pos", "texcoord"});
    gaussianProgram.DetectUniforms({"U_MainTexture"});
    GLuint blurTex = CreateTextureFromFile(S_PATH("resource/image/wood.bmp"));
    GL_CHECK_ERROR;

    FullScreenQuad fsq;
    fsq.Init();

    FBO fboDirectionLight;
    // sRGBA: r 8bit 0~255(0~1.0)
    fboDirectionLight.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboDirectionLight.AttachDepthBuffer("depth", viewW, viewH);
    fboDirectionLight.Finish();

    FBO fboGaussian1;
    fboGaussian1.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboGaussian1.AttachDepthBuffer("depth", viewW, viewH);
    fboGaussian1.Finish();

    FBO fboGaussian2;
    fboGaussian2.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboGaussian2.AttachDepthBuffer("depth", viewW, viewH);
    fboGaussian2.Finish();

    FBO fboGaussian3;
    fboGaussian3.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboGaussian3.AttachDepthBuffer("depth", viewW, viewH);
    fboGaussian3.Finish();

    FBO fboHDR;
    fboHDR.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA16F, viewW, viewH);
    fboHDR.AttachDepthBuffer("depth", viewW, viewH);
    fboHDR.Finish();

    // model
    objModel->InitModel(S_PATH("resource/model/Cube.obj"));

    // 传递参数到shader
    gpuProgSpecular.DetectAttributes({"pos", "texcoord", "normal"});
    gpuProgSpecular.DetectUniforms({"M", "V", "P", "NM", "U_LightPos",
                                    "U_DiffuseLightColor", "U_DiffuseMaterial",
                                    "U_AmbientLightColor", "U_AmbientMaterial",
                                    "U_SpecularLightColor", "U_SpecularMaterial",
                                    "U_EyePos", "U_SpotLightDirect", "U_CutOffAngle"});
    GL_CHECK_ERROR;

    glm::mat4 modelA = glm::translate<float>(-2.0f, 0.0f, -6.0f) * glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    const float WH = static_cast<float>(viewW) / static_cast<float>(viewH);
    glm::mat4 projection = glm::perspective(50.0f, WH, 0.1f, 1000.0f);
    glm::mat4 normalMatrix = glm::inverseTranspose(modelA);
    glm::mat4 viewMatrix1 = glm::lookAt(glm::vec3(-0.5, 1.5f, -3.0f),
                                        glm::vec3(-2.0f, 0.0f, -6.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f));
    float ambientLightColor[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float ambientMaterial[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float diffuseLightColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float diffuseMaterial[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float specularLightColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float specularMaterial[] = {0.8f, 0.8f, 0.8f, 1.0f};
    float lightPos[] = {0.0f, 1.5f, 0.0f, 0.0f};
    float eyePos[] = {0.0f, 0.0f, 0.0f};
    float spotLightDirection[] = {0.0f, -1.0f, 0.0f, 128.0f};
    float spotLightCutOffAngle = 0.0; // degree

    // opengl 环境设置
    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // 开启 alpha 混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    MSG msg;
    glClearColor(0.1f, 0.4f, 0.7f, 1.0f);
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
        // 编译命令
        glUseProgram(gpuProgSpecular.GetGPUProgram());
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("V"), 1, GL_FALSE, glm::value_ptr(viewMatrix1));    // V visual 视口
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影

        // ambient
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_AmbientLightColor"), 1, ambientLightColor);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_AmbientMaterial"), 1, ambientMaterial);

        // diffuse
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_DiffuseLightColor"), 1, diffuseLightColor);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_DiffuseMaterial"), 1, diffuseMaterial);
        GL_CHECK_ERROR;

        // specular
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_SpecularLightColor"), 1, specularLightColor);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_SpecularMaterial"), 1, specularMaterial);
        glUniform3fv(gpuProgSpecular.GetQualfiterLoc("U_EyePos"), 1, eyePos);
        GL_CHECK_ERROR;

        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_SpotLightDirect"), 1, spotLightDirection);
        GL_CHECK_ERROR;

        // 聚光
        lightPos[0] = -1.8f;
        lightPos[1] = 3.0f;
        lightPos[2] = -5.7f;
        lightPos[3] = 1.0f;
        spotLightCutOffAngle = 15.0f;
        glUniform1f(gpuProgSpecular.GetQualfiterLoc("U_CutOffAngle"), spotLightCutOffAngle);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_LightPos"), 1, lightPos);
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(modelA)); // M model,模型视图移动，
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影
        GL_CHECK_ERROR;
        fboHDR.Bind();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        objModel->Bind(gpuProgSpecular.GetQualfiterLoc("pos"), gpuProgSpecular.GetQualfiterLoc("normal"));
        objModel->Draw();
        fboHDR.UnBind();
        GL_CHECK_ERROR;
        glUseProgram(0); // 重置

        // no blur
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(originProgram.GetGPUProgram());
        glUniform1i(originProgram.GetQualfiterLoc("U_MainTexture"), 0);
        glBindTexture(GL_TEXTURE_2D, fboHDR.GetBuffer("color"));
        fsq.DrawToQuarter(originProgram.GetQualfiterLoc("pos"), originProgram.GetQualfiterLoc("texcoord"), 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0); // 重置

        // blur fbo
        fboGaussian1.Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(originProgram.GetGPUProgram());
        glBindTexture(GL_TEXTURE_2D, fboHDR.GetBuffer("color"));
        glUniform1i(originProgram.GetQualfiterLoc("U_MainTexture"), 0);
        fsq.DrawToQuarter(originProgram.GetQualfiterLoc("pos"), originProgram.GetQualfiterLoc("texcoord"), 5);
        GL_CHECK_ERROR;
        fboGaussian1.UnBind();
        
        // blur fbo
        fboGaussian2.Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(verticalGaussProgram.GetGPUProgram());
        glBindTexture(GL_TEXTURE_2D, fboHDR.GetBuffer("color"));
        glUniform1i(verticalGaussProgram.GetQualfiterLoc("U_MainTexture"), 0);
        fsq.DrawToQuarter(verticalGaussProgram.GetQualfiterLoc("pos"), verticalGaussProgram.GetQualfiterLoc("texcoord"), 5);
        GL_CHECK_ERROR;
        fboGaussian2.UnBind();
        


        glUseProgram(hdrProgram.GetGPUProgram());
        glBindTexture(GL_TEXTURE_2D, fboHDR.GetBuffer("color"));
        fsq.DrawToQuarter(hdrProgram.GetQualfiterLoc("pos"), hdrProgram.GetQualfiterLoc("texcoord"), 1);

        // glUseProgram(horizontalGaussProgram.GetGPUProgram());
        // glBindTexture(GL_TEXTURE_2D, fboGaussian2.GetBuffer("color"));
        // fsq.DrawToQuarter(horizontalGaussProgram.GetQualfiterLoc("pos"), horizontalGaussProgram.GetQualfiterLoc("texcoord"), 2);

        // glUseProgram(verticalGaussProgram.GetGPUProgram());
        // glBindTexture(GL_TEXTURE_2D, fboGaussian1.GetBuffer("color"));
        // fsq.DrawToQuarter(verticalGaussProgram.GetQualfiterLoc("pos"), verticalGaussProgram.GetQualfiterLoc("texcoord"), 3);
        // GL_CHECK_ERROR;



        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0); // 重置
        glFinish();
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

    glDisable(GL_BLEND);
    return 0;
}



int GLRenderer::HDRRendering(HWND hwnd, HDC dc, int viewW, int viewH)
{
    // init
    // 0.get Program
    GPUProgram gpuProgSpecular;
    gpuProgSpecular.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/hdrLight.vs"));
    gpuProgSpecular.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/hdrLight.fs"));
    gpuProgSpecular.Link();
    GL_CHECK_ERROR;

    GPUProgram originProgram;
    originProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    originProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/fullscreenQuad.fs"));
    originProgram.Link();
    originProgram.DetectAttributes({"pos", "texcoord"});
    originProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram hdrProgram;
    hdrProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    hdrProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/hdrRenderer.fs"));
    hdrProgram.Link();
    hdrProgram.DetectAttributes({"pos", "texcoord"});
    hdrProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram combineProgram;
    combineProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    combineProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/combineHDRAndNormal.fs"));
    combineProgram.Link();
    combineProgram.DetectAttributes({"pos", "texcoord"});
    combineProgram.DetectUniforms({"U_MainTexture","U_HDRTexture"});
    GL_CHECK_ERROR;

    FullScreenQuad fsq;
    fsq.Init();

    FBO fboDirectionLight;
    // sRGBA: r 8bit 0~255(0~1.0)
    fboDirectionLight.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboDirectionLight.AttachDepthBuffer("depth", viewW, viewH);
    fboDirectionLight.Finish();

    FBO fboGaussian1;
    fboGaussian1.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboGaussian1.AttachDepthBuffer("depth", viewW, viewH);
    fboGaussian1.Finish();

    FBO fboGaussian2;
    fboGaussian2.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboGaussian2.AttachDepthBuffer("depth", viewW, viewH);
    fboGaussian2.Finish();

    FBO fboGaussian3;
    fboGaussian3.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboGaussian3.AttachDepthBuffer("depth", viewW, viewH);
    fboGaussian3.Finish();

    FBO fboHDR;
    fboHDR.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboHDR.AttachColorBuffer("hdrBuffer", GL_COLOR_ATTACHMENT1, GL_RGBA16F, viewW, viewH);
    fboHDR.AttachDepthBuffer("depth", viewW, viewH);
    fboHDR.Finish();
    GL_CHECK_ERROR

    // model
    objModel->InitModel(S_PATH("resource/model/Cube.obj"));

    // 传递参数到shader
    gpuProgSpecular.DetectAttributes({"pos", "texcoord", "normal"});
    gpuProgSpecular.DetectUniforms({"M", "V", "P", "NM", "U_LightPos",
                                    "U_DiffuseLightColor", "U_DiffuseMaterial",
                                    "U_AmbientLightColor", "U_AmbientMaterial",
                                    "U_SpecularLightColor", "U_SpecularMaterial",
                                    "U_EyePos", "U_SpotLightDirect", "U_CutOffAngle"});
    GL_CHECK_ERROR;

    glm::mat4 modelA = glm::translate<float>(-2.0f, 0.0f, -6.0f) * glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    const float WH = static_cast<float>(viewW) / static_cast<float>(viewH);
    glm::mat4 projection = glm::perspective(50.0f, WH, 0.1f, 1000.0f);
    glm::mat4 normalMatrix = glm::inverseTranspose(modelA);
    glm::mat4 viewMatrix1 = glm::lookAt(glm::vec3(-0.5, 1.5f, -3.0f),
                                        glm::vec3(-2.0f, 0.0f, -6.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f));
    float ambientLightColor[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float ambientMaterial[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float diffuseLightColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float diffuseMaterial[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float specularLightColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float specularMaterial[] = {0.8f, 0.8f, 0.8f, 1.0f};
    float lightPos[] = {0.0f, 1.5f, 0.0f, 0.0f};
    float eyePos[] = {0.0f, 0.0f, 0.0f};
    float spotLightDirection[] = {0.0f, -1.0f, 0.0f, 128.0f};
    float spotLightCutOffAngle = 0.0; // degree

    // opengl 环境设置
    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // 开启 alpha 混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    MSG msg;
    glClearColor(0.1f, 0.4f, 0.7f, 1.0f);
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
        // 编译命令
        glUseProgram(gpuProgSpecular.GetGPUProgram());
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("V"), 1, GL_FALSE, glm::value_ptr(viewMatrix1));    // V visual 视口
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影

        // ambient
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_AmbientLightColor"), 1, ambientLightColor);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_AmbientMaterial"), 1, ambientMaterial);

        // diffuse
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_DiffuseLightColor"), 1, diffuseLightColor);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_DiffuseMaterial"), 1, diffuseMaterial);
        GL_CHECK_ERROR;

        // specular
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_SpecularLightColor"), 1, specularLightColor);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_SpecularMaterial"), 1, specularMaterial);
        glUniform3fv(gpuProgSpecular.GetQualfiterLoc("U_EyePos"), 1, eyePos);
        GL_CHECK_ERROR;

        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_SpotLightDirect"), 1, spotLightDirection);
        GL_CHECK_ERROR;

        // 聚光
        lightPos[0] = -1.8f;
        lightPos[1] = 3.0f;
        lightPos[2] = -5.7f;
        lightPos[3] = 1.0f;
        spotLightCutOffAngle = 15.0f;
        glUniform1f(gpuProgSpecular.GetQualfiterLoc("U_CutOffAngle"), spotLightCutOffAngle);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_LightPos"), 1, lightPos);
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(modelA)); // M model,模型视图移动，
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影
        GL_CHECK_ERROR;
        fboHDR.Bind();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        objModel->Bind(gpuProgSpecular.GetQualfiterLoc("pos"), gpuProgSpecular.GetQualfiterLoc("normal"));
        objModel->Draw();
        fboHDR.UnBind();
        GL_CHECK_ERROR;
        glUseProgram(0); // 重置

        // no blur
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(originProgram.GetGPUProgram());
        glBindTexture(GL_TEXTURE_2D, fboHDR.GetBuffer("color"));
        glUniform1i(originProgram.GetQualfiterLoc("U_MainTexture"), 0);
        fsq.DrawToQuarter(originProgram.GetQualfiterLoc("pos"), originProgram.GetQualfiterLoc("texcoord"), 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        glUseProgram(hdrProgram.GetGPUProgram());
        glBindTexture(GL_TEXTURE_2D, fboHDR.GetBuffer("color"));
        glUniform1i(hdrProgram.GetQualfiterLoc("U_MainTexture"), 0);
        fsq.DrawToQuarter(hdrProgram.GetQualfiterLoc("pos"), hdrProgram.GetQualfiterLoc("texcoord"), 1);

        glUseProgram(hdrProgram.GetGPUProgram());
        glBindTexture(GL_TEXTURE_2D, fboHDR.GetBuffer("hdrBuffer"));
        glUniform1i(hdrProgram.GetQualfiterLoc("U_MainTexture"), 0);
        fsq.DrawToQuarter(hdrProgram.GetQualfiterLoc("pos"), hdrProgram.GetQualfiterLoc("texcoord"), 2);
        glBindTexture(GL_TEXTURE_2D, 0);


        // combine program
        glUseProgram(combineProgram.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fboHDR.GetBuffer("color"));
        glUniform1i(combineProgram.GetQualfiterLoc("U_MainTexture"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, fboHDR.GetBuffer("hdrBuffer"));
        glUniform1i(combineProgram.GetQualfiterLoc("U_HDRTexture"), 1);
        fsq.DrawToQuarter(combineProgram.GetQualfiterLoc("pos"), combineProgram.GetQualfiterLoc("texcoord"), 3);
        
        // glUseProgram(verticalGaussProgram.GetGPUProgram());
        // glBindTexture(GL_TEXTURE_2D, fboGaussian1.GetBuffer("color"));
        // fsq.DrawToQuarter(verticalGaussProgram.GetQualfiterLoc("pos"), verticalGaussProgram.GetQualfiterLoc("texcoord"), 3);
        // GL_CHECK_ERROR;
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0); // 重置
        glFinish();
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

    glDisable(GL_BLEND);
    return 0;
}

int GLRenderer::Bloom(HWND hwnd, HDC dc, int viewW, int viewH)
{
    // init
    // 0.get Program
    GPUProgram gpuProgSpecular;
    gpuProgSpecular.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/hdrLight.vs"));
    gpuProgSpecular.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/hdrLight.fs"));
    gpuProgSpecular.Link();
    GL_CHECK_ERROR;

    GPUProgram originProgram;
    originProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    originProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/fullscreenQuad.fs"));
    originProgram.Link();
    originProgram.DetectAttributes({"pos", "texcoord"});
    originProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram hdrProgram;
    hdrProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    hdrProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/hdrRenderer.fs"));
    hdrProgram.Link();
    hdrProgram.DetectAttributes({"pos", "texcoord"});
    hdrProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram combineProgram;
    combineProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    combineProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/combineHDRAndNormal.fs"));
    combineProgram.Link();
    combineProgram.DetectAttributes({"pos", "texcoord"});
    combineProgram.DetectUniforms({"U_MainTexture","U_HDRTexture"});
    GL_CHECK_ERROR;

    GPUProgram bloomProgram;
    bloomProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/light_bloom.vs"));
    bloomProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/light_bloom.fs"));
    bloomProgram.Link();
    bloomProgram.DetectAttributes({"pos"});
    bloomProgram.DetectUniforms({"M", "V", "P"});
    GL_CHECK_ERROR;

    FullScreenQuad fsq;
    fsq.Init();
    this->BlurInit(viewW, viewH);

    FBO fboDirectionLight;
    // sRGBA: r 8bit 0~255(0~1.0)
    fboDirectionLight.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboDirectionLight.AttachDepthBuffer("depth", viewW, viewH);
    fboDirectionLight.Finish();


    FBO fboHDR;
    fboHDR.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboHDR.AttachColorBuffer("hdrBuffer", GL_COLOR_ATTACHMENT1, GL_RGBA16F, viewW, viewH);
    fboHDR.AttachDepthBuffer("depth", viewW, viewH);
    fboHDR.Finish();
    GL_CHECK_ERROR

    // model
    ObjModel cube, quad, sphere;
    cube.InitModel(S_PATH("resource/model/Cube.obj"));
    quad.InitModel(S_PATH("resource/model/Quad.obj"));
    sphere.InitModel(S_PATH("resource/model/Sphere.obj"));

    // 传递参数到shader
    gpuProgSpecular.DetectAttributes({"pos", "texcoord", "normal"});
    gpuProgSpecular.DetectUniforms({"M", "V", "P", "NM", "U_LightPos",
                                    "U_DiffuseLightColor", "U_DiffuseMaterial",
                                    "U_AmbientLightColor", "U_AmbientMaterial",
                                    "U_SpecularLightColor", "U_SpecularMaterial",
                                    "U_EyePos", "U_SpotLightDirect", "U_CutOffAngle"});
    GL_CHECK_ERROR;

    glm::mat4 modelA = glm::translate<float>(-2.0f, 0.0f, -6.0f) * glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    const float WH = static_cast<float>(viewW) / static_cast<float>(viewH);
    glm::mat4 projection = glm::perspective(50.0f, WH, 0.1f, 1000.0f);
    glm::mat4 normalMatrix = glm::inverseTranspose(modelA);
    glm::mat4 viewMatrix1 = glm::lookAt(glm::vec3(4.0, 2.0f, -6.0f),
                                        glm::vec3(-2.0f, 1.0f, -6.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 quadModel = glm::translate<float>(-2.0f, 0.0f, -6.0f) * glm::rotate<float>(-90.0f, 1.0f, 0.0f, 0.0f)
        * glm::scale<float>(4.0f,4.0f,4.0f);
    glm::mat4 quadNormalMat = glm::inverseTranspose(quadModel);

    glm::mat4 sphereModel = glm::translate<float>(-1.8f, 3.0f, -5.7f)*glm::scale<float>(0.2f,0.2f,0.2f);
    glm::mat4 sphereNormalMat = glm::inverseTranspose(sphereModel);

    float ambientLightColor[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float ambientMaterial[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float diffuseLightColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float diffuseMaterial[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float specularLightColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float specularMaterial[] = {0.8f, 0.8f, 0.8f, 1.0f};
    float lightPos[] = {0.0f, 1.5f, 0.0f, 0.0f};
    float eyePos[] = {0.0f, 0.0f, 0.0f};
    float spotLightDirection[] = {0.0f, -1.0f, 0.0f, 128.0f};
    float spotLightCutOffAngle = 0.0; // degree

    // opengl 环境设置
    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // 开启 alpha 混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    MSG msg;
    // glClearColor(0.1f, 0.4f, 0.7f, 1.0f);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

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
        // 编译命令
        glUseProgram(gpuProgSpecular.GetGPUProgram());
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("V"), 1, GL_FALSE, glm::value_ptr(viewMatrix1));    // V visual 视口
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影

        // ambient
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_AmbientLightColor"), 1, ambientLightColor);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_AmbientMaterial"), 1, ambientMaterial);

        // diffuse
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_DiffuseLightColor"), 1, diffuseLightColor);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_DiffuseMaterial"), 1, diffuseMaterial);
        GL_CHECK_ERROR;

        // specular
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_SpecularLightColor"), 1, specularLightColor);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_SpecularMaterial"), 1, specularMaterial);
        glUniform3fv(gpuProgSpecular.GetQualfiterLoc("U_EyePos"), 1, eyePos);
        GL_CHECK_ERROR;

        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_SpotLightDirect"), 1, spotLightDirection);
        GL_CHECK_ERROR;

        // 聚光
        lightPos[0] = -1.8f;
        lightPos[1] = 3.0f;
        lightPos[2] = -5.7f;
        lightPos[3] = 1.0f;
        spotLightCutOffAngle = 0.0f;
        glUniform1f(gpuProgSpecular.GetQualfiterLoc("U_CutOffAngle"), spotLightCutOffAngle);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_LightPos"), 1, lightPos);
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(modelA)); // M model,模型视图移动，
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影
        GL_CHECK_ERROR;
        fboHDR.Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        cube.Bind(gpuProgSpecular.GetQualfiterLoc("pos"), gpuProgSpecular.GetQualfiterLoc("normal"));
        cube.Draw();

        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(quadModel)); // M model,模型视图移动，
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(quadNormalMat));  // 投影
        quad.Bind(gpuProgSpecular.GetQualfiterLoc("pos"), gpuProgSpecular.GetQualfiterLoc("normal"));
        quad.Draw();

        // glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(sphereModel)); // M model,模型视图移动，
        // glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(sphereNormalMat));  // 投影
        // sphere.Bind(gpuProgSpecular.GetQualfiterLoc("pos"), gpuProgSpecular.GetQualfiterLoc("normal"));


        glUseProgram(bloomProgram.GetGPUProgram());
        glUniformMatrix4fv(bloomProgram.GetQualfiterLoc("V"), 1, GL_FALSE, glm::value_ptr(viewMatrix1));    // V visual 视口
        glUniformMatrix4fv(bloomProgram.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        glUniformMatrix4fv(bloomProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(sphereModel)); // M model,模型视图移动，
        sphere.Bind(bloomProgram.GetQualfiterLoc("pos"));
        sphere.Draw();

        fboHDR.UnBind();
        GL_CHECK_ERROR;
        glUseProgram(0); // 重置

        // no blur
        // glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 左上, 未处理图像
        //glUseProgram(originProgram.GetGPUProgram());
        //glBindTexture(GL_TEXTURE_2D, fboHDR.GetBuffer("color"));
        //glUniform1i(originProgram.GetQualfiterLoc("U_MainTexture"), 0);
        //fsq.DrawToQuarter(originProgram.GetQualfiterLoc("pos"), originProgram.GetQualfiterLoc("texcoord"), 0);
        //glBindTexture(GL_TEXTURE_2D, 0);

        // 右上，HDR 图
        //glUseProgram(hdrProgram.GetGPUProgram());
        //glBindTexture(GL_TEXTURE_2D, fboHDR.GetBuffer("hdrBuffer"));
        //glUniform1i(hdrProgram.GetQualfiterLoc("U_MainTexture"), 0);
        //fsq.DrawToQuarter(hdrProgram.GetQualfiterLoc("pos"), hdrProgram.GetQualfiterLoc("texcoord"), 2);
        //glBindTexture(GL_TEXTURE_2D, 0);

        // 左下, HDR 高斯模糊图
        // 高斯模糊
        GLuint blurRes = this->Blur(fsq, fboHDR.GetBuffer("hdrBuffer"), 10);

        // 合成图
        glUseProgram(combineProgram.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fboHDR.GetBuffer("color"));
        glUniform1i(combineProgram.GetQualfiterLoc("U_MainTexture"), 0);
        glActiveTexture(GL_TEXTURE1);
        // glBindTexture(GL_TEXTURE_2D, fboHDR.GetBuffer("hdrBuffer"));
        glBindTexture(GL_TEXTURE_2D, blurRes);
        glUniform1i(combineProgram.GetQualfiterLoc("U_HDRTexture"), 1);
        fsq.DrawToQuarter(combineProgram.GetQualfiterLoc("pos"), combineProgram.GetQualfiterLoc("texcoord"), 0);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0); // 重置
        glFinish();
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

    glDisable(GL_BLEND);
    return 0;
}

int GLRenderer::ImageTest_Blend(HWND hwnd, HDC dc, int viewW, int viewH) // init
{
    // 0.get Program
    GPUProgram gpuProgSpecular;
    gpuProgSpecular.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/hdrLight.vs"));
    gpuProgSpecular.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/hdrLight.fs"));
    gpuProgSpecular.Link();
    GL_CHECK_ERROR;

    GPUProgram originProgram;
    originProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    originProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/fullscreenQuad.fs"));
    originProgram.Link();
    originProgram.DetectAttributes({"pos", "texcoord"});
    originProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram hdrProgram;
    hdrProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    hdrProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/hdrRenderer.fs"));
    hdrProgram.Link();
    hdrProgram.DetectAttributes({"pos", "texcoord"});
    hdrProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram combineProgram;
    combineProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    combineProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/Blend_Normal.fs"));
    combineProgram.Link();
    combineProgram.DetectAttributes({"pos", "texcoord"});
    combineProgram.DetectUniforms({"U_BaseTexture","U_BlendTexture"});
    GL_CHECK_ERROR;

    GPUProgram bloomProgram;
    bloomProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/light_bloom.vs"));
    bloomProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/light_bloom.fs"));
    bloomProgram.Link();
    bloomProgram.DetectAttributes({"pos"});
    bloomProgram.DetectUniforms({"M", "V", "P"});
    GL_CHECK_ERROR;

    FullScreenQuad fsq;
    fsq.Init();
    this->BlurInit(viewW, viewH);

    FBO fboDirectionLight;
    // sRGBA: r 8bit 0~255(0~1.0)
    fboDirectionLight.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboDirectionLight.AttachDepthBuffer("depth", viewW, viewH);
    fboDirectionLight.Finish();


    FBO fboHDR;
    fboHDR.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboHDR.AttachColorBuffer("hdrBuffer", GL_COLOR_ATTACHMENT1, GL_RGBA16F, viewW, viewH);
    fboHDR.AttachDepthBuffer("depth", viewW, viewH);
    fboHDR.Finish();
    GL_CHECK_ERROR

    // model
    ObjModel cube, quad, sphere;
    cube.InitModel(S_PATH("resource/model/Cube.obj"));
    quad.InitModel(S_PATH("resource/model/Quad.obj"));
    sphere.InitModel(S_PATH("resource/model/Sphere.obj"));

    // 传递参数到shader
    gpuProgSpecular.DetectAttributes({"pos", "texcoord", "normal"});
    gpuProgSpecular.DetectUniforms({"M", "V", "P", "NM", "U_LightPos",
                                    "U_DiffuseLightColor", "U_DiffuseMaterial",
                                    "U_AmbientLightColor", "U_AmbientMaterial",
                                    "U_SpecularLightColor", "U_SpecularMaterial",
                                    "U_EyePos", "U_SpotLightDirect", "U_CutOffAngle"});
    GL_CHECK_ERROR;

    glm::mat4 modelA = glm::translate<float>(-2.0f, 0.0f, -6.0f) * glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    const float WH = static_cast<float>(viewW) / static_cast<float>(viewH);
    glm::mat4 projection = glm::perspective(50.0f, WH, 0.1f, 1000.0f);
    glm::mat4 normalMatrix = glm::inverseTranspose(modelA);
    glm::mat4 viewMatrix1 = glm::lookAt(glm::vec3(4.0, 2.0f, -6.0f),
                                        glm::vec3(-2.0f, 1.0f, -6.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 quadModel = glm::translate<float>(-2.0f, 0.0f, -6.0f) * glm::rotate<float>(-90.0f, 1.0f, 0.0f, 0.0f)
        * glm::scale<float>(4.0f,4.0f,4.0f);
    glm::mat4 quadNormalMat = glm::inverseTranspose(quadModel);

    glm::mat4 sphereModel = glm::translate<float>(-1.8f, 3.0f, -5.7f)*glm::scale<float>(0.2f,0.2f,0.2f);
    glm::mat4 sphereNormalMat = glm::inverseTranspose(sphereModel);

    GLuint head = this->CreateTextureFromFile(S_PATH("resource/image/head.png"));
    GLuint grass = this->CreateTextureFromFile(S_PATH("resource/image/grass.png"));

    // opengl 环境设置
    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // 开启 alpha 混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    MSG msg;
    // glClearColor(0.1f, 0.4f, 0.7f, 1.0f);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

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
        // 编译命令
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // glUseProgram(originProgram.GetGPUProgram());
        
        // glUniform1i(originProgram.GetQualfiterLoc("U_MainTexture"), 0);
        // quad.Bind(originProgram.GetQualfiterLoc("pos"));
        // quad.Draw();

        // glUseProgram(bloomProgram.GetGPUProgram());
        // glUniformMatrix4fv(bloomProgram.GetQualfiterLoc("V"), 1, GL_FALSE, glm::value_ptr(viewMatrix1));    // V visual 视口
        // glUniformMatrix4fv(bloomProgram.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        // glUniformMatrix4fv(bloomProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(sphereModel)); // M model,模型视图移动，
        // sphere.Bind(bloomProgram.GetQualfiterLoc("pos"));
        // sphere.Draw();

        // fboHDR.UnBind();
        // GL_CHECK_ERROR;
        // glUseProgram(0); // 重置

        // no blur
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 左上, 未处理图像
        glUseProgram(originProgram.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, head);
        glUniform1i(originProgram.GetQualfiterLoc("U_MainTexture"), 0);
        fsq.DrawToQuarter(originProgram.GetQualfiterLoc("pos"), originProgram.GetQualfiterLoc("texcoord"), 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // 右上，HDR 图
        glUseProgram(originProgram.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, grass);
        glUniform1i(originProgram.GetQualfiterLoc("U_MainTexture"), 0);
        fsq.DrawToQuarter(originProgram.GetQualfiterLoc("pos"), originProgram.GetQualfiterLoc("texcoord"), 2);
        glBindTexture(GL_TEXTURE_2D, 0);

        // 左下, HDR 高斯模糊图
        // 高斯模糊
        GLuint blurRes = this->Blur(fsq, head, 5);
        glUseProgram(originProgram.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, blurRes);
        glUniform1i(originProgram.GetQualfiterLoc("U_MainTexture"), 0);
        fsq.DrawToQuarter(originProgram.GetQualfiterLoc("pos"), originProgram.GetQualfiterLoc("texcoord"), 1);
        glBindTexture(GL_TEXTURE_2D, 0);

        // 合成图
        glUseProgram(combineProgram.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, head); // base
        glUniform1i(combineProgram.GetQualfiterLoc("U_BaseTexture"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, grass); // blend
        glUniform1i(combineProgram.GetQualfiterLoc("U_BlendTexture"), 1);
        fsq.DrawToQuarter(combineProgram.GetQualfiterLoc("pos"), combineProgram.GetQualfiterLoc("texcoord"), 3);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0); // 重置
        glFinish();
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

    glDisable(GL_BLEND);
    return 0;
}

int GLRenderer::ImageTest_lighterOrDarker(HWND hwnd, HDC dc, int viewW, int viewH) // init
{
    // 0.get Program
    GPUProgram gpuProgSpecular;
    gpuProgSpecular.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/hdrLight.vs"));
    gpuProgSpecular.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/hdrLight.fs"));
    gpuProgSpecular.Link();
    GL_CHECK_ERROR;

    GPUProgram originProgram;
    originProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    originProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/fullscreenQuad.fs"));
    originProgram.Link();
    originProgram.DetectAttributes({"pos", "texcoord"});
    originProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram hdrProgram;
    hdrProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    hdrProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/hdrRenderer.fs"));
    hdrProgram.Link();
    hdrProgram.DetectAttributes({"pos", "texcoord"});
    hdrProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram combineProgram;
    combineProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    combineProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/lighter.fs"));
    combineProgram.Link();
    combineProgram.DetectAttributes({"pos", "texcoord"});
    combineProgram.DetectUniforms({"U_BaseTexture","U_BlendTexture"});
    GL_CHECK_ERROR;

    GPUProgram bloomProgram;
    bloomProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/light_bloom.vs"));
    bloomProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/light_bloom.fs"));
    bloomProgram.Link();
    bloomProgram.DetectAttributes({"pos"});
    bloomProgram.DetectUniforms({"M", "V", "P"});
    GL_CHECK_ERROR;

    FullScreenQuad fsq;
    fsq.Init();
    this->BlurInit(viewW, viewH);

    FBO fboDirectionLight;
    // sRGBA: r 8bit 0~255(0~1.0)
    fboDirectionLight.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboDirectionLight.AttachDepthBuffer("depth", viewW, viewH);
    fboDirectionLight.Finish();


    FBO fboHDR;
    fboHDR.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboHDR.AttachColorBuffer("hdrBuffer", GL_COLOR_ATTACHMENT1, GL_RGBA16F, viewW, viewH);
    fboHDR.AttachDepthBuffer("depth", viewW, viewH);
    fboHDR.Finish();
    GL_CHECK_ERROR

    // model
    ObjModel cube, quad, sphere;
    cube.InitModel(S_PATH("resource/model/Cube.obj"));
    quad.InitModel(S_PATH("resource/model/Quad.obj"));
    sphere.InitModel(S_PATH("resource/model/Sphere.obj"));

    // 传递参数到shader
    gpuProgSpecular.DetectAttributes({"pos", "texcoord", "normal"});
    gpuProgSpecular.DetectUniforms({"M", "V", "P", "NM", "U_LightPos",
                                    "U_DiffuseLightColor", "U_DiffuseMaterial",
                                    "U_AmbientLightColor", "U_AmbientMaterial",
                                    "U_SpecularLightColor", "U_SpecularMaterial",
                                    "U_EyePos", "U_SpotLightDirect", "U_CutOffAngle"});
    GL_CHECK_ERROR;

    glm::mat4 modelA = glm::translate<float>(-2.0f, 0.0f, -6.0f) * glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    const float WH = static_cast<float>(viewW) / static_cast<float>(viewH);
    glm::mat4 projection = glm::perspective(50.0f, WH, 0.1f, 1000.0f);
    glm::mat4 normalMatrix = glm::inverseTranspose(modelA);
    glm::mat4 viewMatrix1 = glm::lookAt(glm::vec3(4.0, 2.0f, -6.0f),
                                        glm::vec3(-2.0f, 1.0f, -6.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 quadModel = glm::translate<float>(-2.0f, 0.0f, -6.0f) * glm::rotate<float>(-90.0f, 1.0f, 0.0f, 0.0f)
        * glm::scale<float>(4.0f,4.0f,4.0f);
    glm::mat4 quadNormalMat = glm::inverseTranspose(quadModel);

    glm::mat4 sphereModel = glm::translate<float>(-1.8f, 3.0f, -5.7f)*glm::scale<float>(0.2f,0.2f,0.2f);
    glm::mat4 sphereNormalMat = glm::inverseTranspose(sphereModel);

    GLuint head = this->CreateTextureFromFile(S_PATH("resource/image/wood.bmp"));
    GLuint grass = this->CreateTextureFromFile(S_PATH("resource/image/earth.bmp"));

    // opengl 环境设置
    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // 开启 alpha 混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    MSG msg;
    // glClearColor(0.1f, 0.4f, 0.7f, 1.0f);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

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
        // 编译命令
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // glUseProgram(originProgram.GetGPUProgram());
        
        // glUniform1i(originProgram.GetQualfiterLoc("U_MainTexture"), 0);
        // quad.Bind(originProgram.GetQualfiterLoc("pos"));
        // quad.Draw();

        // glUseProgram(bloomProgram.GetGPUProgram());
        // glUniformMatrix4fv(bloomProgram.GetQualfiterLoc("V"), 1, GL_FALSE, glm::value_ptr(viewMatrix1));    // V visual 视口
        // glUniformMatrix4fv(bloomProgram.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        // glUniformMatrix4fv(bloomProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(sphereModel)); // M model,模型视图移动，
        // sphere.Bind(bloomProgram.GetQualfiterLoc("pos"));
        // sphere.Draw();

        // fboHDR.UnBind();
        // GL_CHECK_ERROR;
        // glUseProgram(0); // 重置

        // no blur
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 左上, 未处理图像
        glUseProgram(originProgram.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, head);
        glUniform1i(originProgram.GetQualfiterLoc("U_MainTexture"), 0);
        fsq.DrawToQuarter(originProgram.GetQualfiterLoc("pos"), originProgram.GetQualfiterLoc("texcoord"), 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // 右上，HDR 图
        glUseProgram(originProgram.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, grass);
        glUniform1i(originProgram.GetQualfiterLoc("U_MainTexture"), 0);
        fsq.DrawToQuarter(originProgram.GetQualfiterLoc("pos"), originProgram.GetQualfiterLoc("texcoord"), 2);
        glBindTexture(GL_TEXTURE_2D, 0);

        // 左下, HDR 高斯模糊图
        // 高斯模糊
        GLuint blurRes = this->Blur(fsq, head, 5);
        glUseProgram(originProgram.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, blurRes);
        glUniform1i(originProgram.GetQualfiterLoc("U_MainTexture"), 0);
        fsq.DrawToQuarter(originProgram.GetQualfiterLoc("pos"), originProgram.GetQualfiterLoc("texcoord"), 1);
        glBindTexture(GL_TEXTURE_2D, 0);

        // 合成图
        glUseProgram(combineProgram.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, head); // base
        glUniform1i(combineProgram.GetQualfiterLoc("U_BaseTexture"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, grass); // blend
        glUniform1i(combineProgram.GetQualfiterLoc("U_BlendTexture"), 1);
        fsq.DrawToQuarter(combineProgram.GetQualfiterLoc("pos"), combineProgram.GetQualfiterLoc("texcoord"), 3);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0); // 重置
        glFinish();
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

    glDisable(GL_BLEND);
    return 0;
}

int GLRenderer::ImageTest_ZPDD(HWND hwnd, HDC dc, int viewW, int viewH)
{
    // 0.get Program
    GPUProgram gpuProgSpecular;
    gpuProgSpecular.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/hdrLight.vs"));
    gpuProgSpecular.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/hdrLight.fs"));
    gpuProgSpecular.Link();
    GL_CHECK_ERROR;

    GPUProgram originProgram;
    originProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    originProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/fullscreenQuad.fs"));
    originProgram.Link();
    originProgram.DetectAttributes({"pos", "texcoord"});
    originProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram hdrProgram;
    hdrProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    hdrProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/hdrRenderer.fs"));
    hdrProgram.Link();
    hdrProgram.DetectAttributes({"pos", "texcoord"});
    hdrProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram combineProgram1;
    combineProgram1.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    combineProgram1.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/zpdd.fs"));
    combineProgram1.Link();
    combineProgram1.DetectAttributes({"pos", "texcoord"});
    combineProgram1.DetectUniforms({"U_BaseTexture","U_BlendTexture"});
    GL_CHECK_ERROR;

    GPUProgram combineProgram2;
    combineProgram2.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    combineProgram2.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/zpdd_inverse.fs"));
    combineProgram2.Link();
    combineProgram2.DetectAttributes({"pos", "texcoord"});
    combineProgram2.DetectUniforms({"U_BaseTexture","U_BlendTexture"});
    GL_CHECK_ERROR;

    GPUProgram combineProgram3; // 颜色减淡
    combineProgram3.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    combineProgram3.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/moto_lighter.fs"));
    combineProgram3.Link();
    combineProgram3.DetectAttributes({"pos", "texcoord"});
    combineProgram3.DetectUniforms({"U_BaseTexture","U_BlendTexture"});
    GL_CHECK_ERROR;

    GPUProgram combineProgram4; // 颜色加深
    combineProgram4.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    combineProgram4.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/moto_darker.fs"));
    combineProgram4.Link();
    combineProgram4.DetectAttributes({"pos", "texcoord"});
    combineProgram4.DetectUniforms({"U_BaseTexture","U_BlendTexture"});
    GL_CHECK_ERROR;

    FullScreenQuad fsq;
    fsq.Init();
    this->BlurInit(viewW, viewH);

    FBO fboDirectionLight;
    // sRGBA: r 8bit 0~255(0~1.0)
    fboDirectionLight.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboDirectionLight.AttachDepthBuffer("depth", viewW, viewH);
    fboDirectionLight.Finish();


    FBO fboHDR;
    fboHDR.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboHDR.AttachColorBuffer("hdrBuffer", GL_COLOR_ATTACHMENT1, GL_RGBA16F, viewW, viewH);
    fboHDR.AttachDepthBuffer("depth", viewW, viewH);
    fboHDR.Finish();
    GL_CHECK_ERROR

    // model
    ObjModel cube, quad, sphere;
    cube.InitModel(S_PATH("resource/model/Cube.obj"));
    quad.InitModel(S_PATH("resource/model/Quad.obj"));
    sphere.InitModel(S_PATH("resource/model/Sphere.obj"));

    // 传递参数到shader
    gpuProgSpecular.DetectAttributes({"pos", "texcoord", "normal"});
    gpuProgSpecular.DetectUniforms({"M", "V", "P", "NM", "U_LightPos",
                                    "U_DiffuseLightColor", "U_DiffuseMaterial",
                                    "U_AmbientLightColor", "U_AmbientMaterial",
                                    "U_SpecularLightColor", "U_SpecularMaterial",
                                    "U_EyePos", "U_SpotLightDirect", "U_CutOffAngle"});
    GL_CHECK_ERROR;

    glm::mat4 modelA = glm::translate<float>(-2.0f, 0.0f, -6.0f) * glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    const float WH = static_cast<float>(viewW) / static_cast<float>(viewH);
    glm::mat4 projection = glm::perspective(50.0f, WH, 0.1f, 1000.0f);
    glm::mat4 normalMatrix = glm::inverseTranspose(modelA);
    glm::mat4 viewMatrix1 = glm::lookAt(glm::vec3(4.0, 2.0f, -6.0f),
                                        glm::vec3(-2.0f, 1.0f, -6.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 quadModel = glm::translate<float>(-2.0f, 0.0f, -6.0f) * glm::rotate<float>(-90.0f, 1.0f, 0.0f, 0.0f)
        * glm::scale<float>(4.0f,4.0f,4.0f);
    glm::mat4 quadNormalMat = glm::inverseTranspose(quadModel);

    glm::mat4 sphereModel = glm::translate<float>(-1.8f, 3.0f, -5.7f)*glm::scale<float>(0.2f,0.2f,0.2f);
    glm::mat4 sphereNormalMat = glm::inverseTranspose(sphereModel);

    GLuint head = this->CreateTextureFromFile(S_PATH("resource/image/wood.bmp"));
    GLuint grass = this->CreateTextureFromFile(S_PATH("resource/image/earth.bmp"));

    // opengl 环境设置
    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // 开启 alpha 混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    MSG msg;
    // glClearColor(0.1f, 0.4f, 0.7f, 1.0f);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

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
        // 编译命令
        // no blur
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 左上, 正片叠底
        glUseProgram(combineProgram1.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, head); // base
        glUniform1i(combineProgram1.GetQualfiterLoc("U_BaseTexture"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, grass); // blend
        glUniform1i(combineProgram1.GetQualfiterLoc("U_BlendTexture"), 1);
        fsq.DrawToQuarter(combineProgram1.GetQualfiterLoc("pos"), combineProgram1.GetQualfiterLoc("texcoord"), 0);

        // 右上，反正片叠底
        glUseProgram(combineProgram2.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, head); // base
        glUniform1i(combineProgram2.GetQualfiterLoc("U_BaseTexture"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, grass); // blend
        glUniform1i(combineProgram2.GetQualfiterLoc("U_BlendTexture"), 1);
        fsq.DrawToQuarter(combineProgram2.GetQualfiterLoc("pos"), combineProgram2.GetQualfiterLoc("texcoord"), 2);
        glBindTexture(GL_TEXTURE_2D, 0);


        // 左下，颜色减淡
        glUseProgram(combineProgram3.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, head); // base
        glUniform1i(combineProgram3.GetQualfiterLoc("U_BaseTexture"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, grass); // blend
        glUniform1i(combineProgram3.GetQualfiterLoc("U_BlendTexture"), 1);
        fsq.DrawToQuarter(combineProgram3.GetQualfiterLoc("pos"), combineProgram3.GetQualfiterLoc("texcoord"), 1);
        glBindTexture(GL_TEXTURE_2D, 0);

        // 右下，颜色加深
        glUseProgram(combineProgram4.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, head); // base
        glUniform1i(combineProgram4.GetQualfiterLoc("U_BaseTexture"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, grass); // blend
        glUniform1i(combineProgram4.GetQualfiterLoc("U_BlendTexture"), 1);
        fsq.DrawToQuarter(combineProgram4.GetQualfiterLoc("pos"), combineProgram4.GetQualfiterLoc("texcoord"), 3);
        glBindTexture(GL_TEXTURE_2D, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0); // 重置
        glFinish();
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

    glDisable(GL_BLEND);
    return 0;
}

int GLRenderer::ImageTest_RouGuang(HWND hwnd, HDC dc, int viewW, int viewH)
{
    // 0.get Program
    GPUProgram gpuProgSpecular;
    gpuProgSpecular.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/hdrLight.vs"));
    gpuProgSpecular.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/hdrLight.fs"));
    gpuProgSpecular.Link();
    GL_CHECK_ERROR;

    GPUProgram originProgram;
    originProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    originProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/fullscreenQuad.fs"));
    originProgram.Link();
    originProgram.DetectAttributes({"pos", "texcoord"});
    originProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram hdrProgram;
    hdrProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    hdrProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/hdrRenderer.fs"));
    hdrProgram.Link();
    hdrProgram.DetectAttributes({"pos", "texcoord"});
    hdrProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram combineProgram1;
    combineProgram1.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    combineProgram1.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/img_process/add.fs"));
    combineProgram1.Link();
    combineProgram1.DetectAttributes({"pos", "texcoord"});
    combineProgram1.DetectUniforms({"U_BaseTexture","U_BlendTexture"});
    GL_CHECK_ERROR;

    GPUProgram combineProgram2;
    combineProgram2.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    combineProgram2.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/img_process/minus.fs"));
    combineProgram2.Link();
    combineProgram2.DetectAttributes({"pos", "texcoord"});
    combineProgram2.DetectUniforms({"U_BaseTexture","U_BlendTexture"});
    GL_CHECK_ERROR;

    GPUProgram combineProgram3; // 颜色减淡
    combineProgram3.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    combineProgram3.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/img_process/rouguang.fs"));
    combineProgram3.Link();
    combineProgram3.DetectAttributes({"pos", "texcoord"});
    combineProgram3.DetectUniforms({"U_BaseTexture","U_BlendTexture"});
    GL_CHECK_ERROR;

    GPUProgram combineProgram4; // 颜色加深
    combineProgram4.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    combineProgram4.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/img_process/overlay.fs"));
    combineProgram4.Link();
    combineProgram4.DetectAttributes({"pos", "texcoord"});
    combineProgram4.DetectUniforms({"U_BaseTexture","U_BlendTexture"});
    GL_CHECK_ERROR;

    FullScreenQuad fsq;
    fsq.Init();
    this->BlurInit(viewW, viewH);

    FBO fboDirectionLight;
    // sRGBA: r 8bit 0~255(0~1.0)
    fboDirectionLight.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboDirectionLight.AttachDepthBuffer("depth", viewW, viewH);
    fboDirectionLight.Finish();


    FBO fboHDR;
    fboHDR.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboHDR.AttachColorBuffer("hdrBuffer", GL_COLOR_ATTACHMENT1, GL_RGBA16F, viewW, viewH);
    fboHDR.AttachDepthBuffer("depth", viewW, viewH);
    fboHDR.Finish();
    GL_CHECK_ERROR

    // model
    ObjModel cube, quad, sphere;
    cube.InitModel(S_PATH("resource/model/Cube.obj"));
    quad.InitModel(S_PATH("resource/model/Quad.obj"));
    sphere.InitModel(S_PATH("resource/model/Sphere.obj"));

    // 传递参数到shader
    gpuProgSpecular.DetectAttributes({"pos", "texcoord", "normal"});
    gpuProgSpecular.DetectUniforms({"M", "V", "P", "NM", "U_LightPos",
                                    "U_DiffuseLightColor", "U_DiffuseMaterial",
                                    "U_AmbientLightColor", "U_AmbientMaterial",
                                    "U_SpecularLightColor", "U_SpecularMaterial",
                                    "U_EyePos", "U_SpotLightDirect", "U_CutOffAngle"});
    GL_CHECK_ERROR;

    glm::mat4 modelA = glm::translate<float>(-2.0f, 0.0f, -6.0f) * glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    const float WH = static_cast<float>(viewW) / static_cast<float>(viewH);
    glm::mat4 projection = glm::perspective(50.0f, WH, 0.1f, 1000.0f);
    glm::mat4 normalMatrix = glm::inverseTranspose(modelA);
    glm::mat4 viewMatrix1 = glm::lookAt(glm::vec3(4.0, 2.0f, -6.0f),
                                        glm::vec3(-2.0f, 1.0f, -6.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 quadModel = glm::translate<float>(-2.0f, 0.0f, -6.0f) * glm::rotate<float>(-90.0f, 1.0f, 0.0f, 0.0f)
        * glm::scale<float>(4.0f,4.0f,4.0f);
    glm::mat4 quadNormalMat = glm::inverseTranspose(quadModel);

    glm::mat4 sphereModel = glm::translate<float>(-1.8f, 3.0f, -5.7f)*glm::scale<float>(0.2f,0.2f,0.2f);
    glm::mat4 sphereNormalMat = glm::inverseTranspose(sphereModel);

    GLuint head = this->CreateTextureFromFile(S_PATH("resource/image/wood.bmp"));
    GLuint grass = this->CreateTextureFromFile(S_PATH("resource/image/stone.bmp"));

    // opengl 环境设置
    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // 开启 alpha 混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    MSG msg;
    // glClearColor(0.1f, 0.4f, 0.7f, 1.0f);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

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
        // 编译命令
        // no blur
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 左上, 正片叠底
        glUseProgram(combineProgram1.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, head); // base
        glUniform1i(combineProgram1.GetQualfiterLoc("U_BaseTexture"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, grass); // blend
        glUniform1i(combineProgram1.GetQualfiterLoc("U_BlendTexture"), 1);
        fsq.DrawToQuarter(combineProgram1.GetQualfiterLoc("pos"), combineProgram1.GetQualfiterLoc("texcoord"), 0);

        // 右上，反正片叠底
        glUseProgram(combineProgram2.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, head); // base
        glUniform1i(combineProgram2.GetQualfiterLoc("U_BaseTexture"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, grass); // blend
        glUniform1i(combineProgram2.GetQualfiterLoc("U_BlendTexture"), 1);
        fsq.DrawToQuarter(combineProgram2.GetQualfiterLoc("pos"), combineProgram2.GetQualfiterLoc("texcoord"), 2);
        glBindTexture(GL_TEXTURE_2D, 0);


        // 左下，颜色减淡
        glUseProgram(combineProgram3.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, head); // base
        glUniform1i(combineProgram3.GetQualfiterLoc("U_BaseTexture"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, grass); // blend
        glUniform1i(combineProgram3.GetQualfiterLoc("U_BlendTexture"), 1);
        fsq.DrawToQuarter(combineProgram3.GetQualfiterLoc("pos"), combineProgram3.GetQualfiterLoc("texcoord"), 1);
        glBindTexture(GL_TEXTURE_2D, 0);

        // 右下，颜色加深
        glUseProgram(combineProgram4.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, head); // base
        glUniform1i(combineProgram4.GetQualfiterLoc("U_BaseTexture"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, grass); // blend
        glUniform1i(combineProgram4.GetQualfiterLoc("U_BlendTexture"), 1);
        fsq.DrawToQuarter(combineProgram4.GetQualfiterLoc("pos"), combineProgram4.GetQualfiterLoc("texcoord"), 3);
        glBindTexture(GL_TEXTURE_2D, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0); // 重置
        glFinish();
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

    glDisable(GL_BLEND);
    return 0;
}

int GLRenderer::ImageTest_QiangGuang(HWND hwnd, HDC dc, int viewW, int viewH)
{
    // 0.get Program
    GPUProgram gpuProgSpecular;
    gpuProgSpecular.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/hdrLight.vs"));
    gpuProgSpecular.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/hdrLight.fs"));
    gpuProgSpecular.Link();
    GL_CHECK_ERROR;

    GPUProgram originProgram;
    originProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    originProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/fullscreenQuad.fs"));
    originProgram.Link();
    originProgram.DetectAttributes({"pos", "texcoord"});
    originProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram hdrProgram;
    hdrProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    hdrProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/hdrRenderer.fs"));
    hdrProgram.Link();
    hdrProgram.DetectAttributes({"pos", "texcoord"});
    hdrProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram combineProgram1;
    combineProgram1.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    combineProgram1.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/img_process/QiangGuang.fs"));
    combineProgram1.Link();
    combineProgram1.DetectAttributes({"pos", "texcoord"});
    combineProgram1.DetectUniforms({"U_BaseTexture","U_BlendTexture"});
    GL_CHECK_ERROR;

    GPUProgram combineProgram2;
    combineProgram2.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    combineProgram2.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/img_process/ChaZhi.fs"));
    combineProgram2.Link();
    combineProgram2.DetectAttributes({"pos", "texcoord"});
    combineProgram2.DetectUniforms({"U_BaseTexture","U_BlendTexture"});
    GL_CHECK_ERROR;

    GPUProgram combineProgram3;
    combineProgram3.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    combineProgram3.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/img_process/ChaZhi_Inverse.fs"));
    combineProgram3.Link();
    combineProgram3.DetectAttributes({"pos", "texcoord"});
    combineProgram3.DetectUniforms({"U_BaseTexture","U_BlendTexture"});
    GL_CHECK_ERROR;

    GPUProgram combineProgram4;
    combineProgram4.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    combineProgram4.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/img_process/PaiChu.fs"));
    combineProgram4.Link();
    combineProgram4.DetectAttributes({"pos", "texcoord"});
    combineProgram4.DetectUniforms({"U_BaseTexture","U_BlendTexture"});
    GL_CHECK_ERROR;

    FullScreenQuad fsq;
    fsq.Init();
    this->BlurInit(viewW, viewH);

    FBO fboDirectionLight;
    // sRGBA: r 8bit 0~255(0~1.0)
    fboDirectionLight.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboDirectionLight.AttachDepthBuffer("depth", viewW, viewH);
    fboDirectionLight.Finish();


    FBO fboHDR;
    fboHDR.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboHDR.AttachColorBuffer("hdrBuffer", GL_COLOR_ATTACHMENT1, GL_RGBA16F, viewW, viewH);
    fboHDR.AttachDepthBuffer("depth", viewW, viewH);
    fboHDR.Finish();
    GL_CHECK_ERROR

    // model
    ObjModel cube, quad, sphere;
    cube.InitModel(S_PATH("resource/model/Cube.obj"));
    quad.InitModel(S_PATH("resource/model/Quad.obj"));
    sphere.InitModel(S_PATH("resource/model/Sphere.obj"));

    // 传递参数到shader
    gpuProgSpecular.DetectAttributes({"pos", "texcoord", "normal"});
    gpuProgSpecular.DetectUniforms({"M", "V", "P", "NM", "U_LightPos",
                                    "U_DiffuseLightColor", "U_DiffuseMaterial",
                                    "U_AmbientLightColor", "U_AmbientMaterial",
                                    "U_SpecularLightColor", "U_SpecularMaterial",
                                    "U_EyePos", "U_SpotLightDirect", "U_CutOffAngle"});
    GL_CHECK_ERROR;

    glm::mat4 modelA = glm::translate<float>(-2.0f, 0.0f, -6.0f) * glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    const float WH = static_cast<float>(viewW) / static_cast<float>(viewH);
    glm::mat4 projection = glm::perspective(50.0f, WH, 0.1f, 1000.0f);
    glm::mat4 normalMatrix = glm::inverseTranspose(modelA);
    glm::mat4 viewMatrix1 = glm::lookAt(glm::vec3(4.0, 2.0f, -6.0f),
                                        glm::vec3(-2.0f, 1.0f, -6.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 quadModel = glm::translate<float>(-2.0f, 0.0f, -6.0f) * glm::rotate<float>(-90.0f, 1.0f, 0.0f, 0.0f)
        * glm::scale<float>(4.0f,4.0f,4.0f);
    glm::mat4 quadNormalMat = glm::inverseTranspose(quadModel);

    glm::mat4 sphereModel = glm::translate<float>(-1.8f, 3.0f, -5.7f)*glm::scale<float>(0.2f,0.2f,0.2f);
    glm::mat4 sphereNormalMat = glm::inverseTranspose(sphereModel);

    GLuint head = this->CreateTextureFromFile(S_PATH("resource/image/wood.bmp"));
    GLuint grass = this->CreateTextureFromFile(S_PATH("resource/image/stone.bmp"));

    // opengl 环境设置
    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // 开启 alpha 混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    MSG msg;
    // glClearColor(0.1f, 0.4f, 0.7f, 1.0f);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

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
        // 编译命令
        // no blur
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 左上, 正片叠底
        glUseProgram(combineProgram1.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, head); // base
        glUniform1i(combineProgram1.GetQualfiterLoc("U_BaseTexture"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, grass); // blend
        glUniform1i(combineProgram1.GetQualfiterLoc("U_BlendTexture"), 1);
        fsq.DrawToQuarter(combineProgram1.GetQualfiterLoc("pos"), combineProgram1.GetQualfiterLoc("texcoord"), 0);

        // 右上，反正片叠底
        glUseProgram(combineProgram2.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, head); // base
        glUniform1i(combineProgram2.GetQualfiterLoc("U_BaseTexture"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, grass); // blend
        glUniform1i(combineProgram2.GetQualfiterLoc("U_BlendTexture"), 1);
        fsq.DrawToQuarter(combineProgram2.GetQualfiterLoc("pos"), combineProgram2.GetQualfiterLoc("texcoord"), 2);
        glBindTexture(GL_TEXTURE_2D, 0);


        // 左下，颜色减淡
        glUseProgram(combineProgram3.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, head); // base
        glUniform1i(combineProgram3.GetQualfiterLoc("U_BaseTexture"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, grass); // blend
        glUniform1i(combineProgram3.GetQualfiterLoc("U_BlendTexture"), 1);
        fsq.DrawToQuarter(combineProgram3.GetQualfiterLoc("pos"), combineProgram3.GetQualfiterLoc("texcoord"), 1);
        glBindTexture(GL_TEXTURE_2D, 0);

        // 右下，颜色加深
        glUseProgram(combineProgram4.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, head); // base
        glUniform1i(combineProgram4.GetQualfiterLoc("U_BaseTexture"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, grass); // blend
        glUniform1i(combineProgram4.GetQualfiterLoc("U_BlendTexture"), 1);
        fsq.DrawToQuarter(combineProgram4.GetQualfiterLoc("pos"), combineProgram4.GetQualfiterLoc("texcoord"), 3);
        glBindTexture(GL_TEXTURE_2D, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0); // 重置
        glFinish();
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

    glDisable(GL_BLEND);
    return 0;
}


int GLRenderer::ImageTest_Smooth(HWND hwnd, HDC dc, int viewW, int viewH)
{
    // 0.get Program
    GPUProgram gpuProgSpecular;
    gpuProgSpecular.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/hdrLight.vs"));
    gpuProgSpecular.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/hdrLight.fs"));
    gpuProgSpecular.Link();
    GL_CHECK_ERROR;

    GPUProgram originProgram;
    originProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    originProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/fullscreenQuad.fs"));
    originProgram.Link();
    originProgram.DetectAttributes({"pos", "texcoord"});
    originProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram hdrProgram;
    hdrProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    hdrProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/hdrRenderer.fs"));
    hdrProgram.Link();
    hdrProgram.DetectAttributes({"pos", "texcoord"});
    hdrProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram combineProgram1;
    combineProgram1.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    combineProgram1.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/img_process/smooth.fs"));
    combineProgram1.Link();
    combineProgram1.DetectAttributes({"pos", "texcoord"});
    combineProgram1.DetectUniforms({"U_BaseTexture","U_BlendTexture"});
    GL_CHECK_ERROR;

    GPUProgram combineProgram2;
    combineProgram2.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    combineProgram2.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/img_process/sharpen.fs"));
    combineProgram2.Link();
    combineProgram2.DetectAttributes({"pos", "texcoord"});
    combineProgram2.DetectUniforms({"U_BaseTexture","U_BlendTexture"});
    GL_CHECK_ERROR;

    GPUProgram combineProgram3;
    combineProgram3.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    combineProgram3.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/img_process/edgeDetect.fs"));
    combineProgram3.Link();
    combineProgram3.DetectAttributes({"pos", "texcoord"});
    combineProgram3.DetectUniforms({"U_BaseTexture","U_BlendTexture"});
    GL_CHECK_ERROR;

    GPUProgram combineProgram4;
    combineProgram4.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    combineProgram4.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/img_process/PaiChu.fs"));
    combineProgram4.Link();
    combineProgram4.DetectAttributes({"pos", "texcoord"});
    combineProgram4.DetectUniforms({"U_BaseTexture","U_BlendTexture"});
    GL_CHECK_ERROR;

    FullScreenQuad fsq;
    fsq.Init();
    this->BlurInit(viewW, viewH);

    FBO fboDirectionLight;
    // sRGBA: r 8bit 0~255(0~1.0)
    fboDirectionLight.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboDirectionLight.AttachDepthBuffer("depth", viewW, viewH);
    fboDirectionLight.Finish();


    FBO fboHDR;
    fboHDR.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboHDR.AttachColorBuffer("hdrBuffer", GL_COLOR_ATTACHMENT1, GL_RGBA16F, viewW, viewH);
    fboHDR.AttachDepthBuffer("depth", viewW, viewH);
    fboHDR.Finish();
    GL_CHECK_ERROR

    // model
    ObjModel cube, quad, sphere;
    cube.InitModel(S_PATH("resource/model/Cube.obj"));
    quad.InitModel(S_PATH("resource/model/Quad.obj"));
    sphere.InitModel(S_PATH("resource/model/Sphere.obj"));

    // 传递参数到shader
    gpuProgSpecular.DetectAttributes({"pos", "texcoord", "normal"});
    gpuProgSpecular.DetectUniforms({"M", "V", "P", "NM", "U_LightPos",
                                    "U_DiffuseLightColor", "U_DiffuseMaterial",
                                    "U_AmbientLightColor", "U_AmbientMaterial",
                                    "U_SpecularLightColor", "U_SpecularMaterial",
                                    "U_EyePos", "U_SpotLightDirect", "U_CutOffAngle"});
    GL_CHECK_ERROR;

    glm::mat4 modelA = glm::translate<float>(-2.0f, 0.0f, -6.0f) * glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    const float WH = static_cast<float>(viewW) / static_cast<float>(viewH);
    glm::mat4 projection = glm::perspective(50.0f, WH, 0.1f, 1000.0f);
    glm::mat4 normalMatrix = glm::inverseTranspose(modelA);
    glm::mat4 viewMatrix1 = glm::lookAt(glm::vec3(4.0, 2.0f, -6.0f),
                                        glm::vec3(-2.0f, 1.0f, -6.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 quadModel = glm::translate<float>(-2.0f, 0.0f, -6.0f) * glm::rotate<float>(-90.0f, 1.0f, 0.0f, 0.0f)
        * glm::scale<float>(4.0f,4.0f,4.0f);
    glm::mat4 quadNormalMat = glm::inverseTranspose(quadModel);

    glm::mat4 sphereModel = glm::translate<float>(-1.8f, 3.0f, -5.7f)*glm::scale<float>(0.2f,0.2f,0.2f);
    glm::mat4 sphereNormalMat = glm::inverseTranspose(sphereModel);

    GLuint head = this->CreateTextureFromFile(S_PATH("resource/image/wood.bmp"));
    GLuint grass = this->CreateTextureFromFile(S_PATH("resource/image/stone.bmp"));

    // opengl 环境设置
    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // 开启 alpha 混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    MSG msg;
    // glClearColor(0.1f, 0.4f, 0.7f, 1.0f);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

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
        // 编译命令
        // no blur
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 左上，原图
        glUseProgram(originProgram.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, head); // base
        glUniform1i(originProgram.GetQualfiterLoc("U_MainTexture"), 0);
        fsq.DrawToQuarter(originProgram.GetQualfiterLoc("pos"), originProgram.GetQualfiterLoc("texcoord"), 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // 右上, 平滑
        glUseProgram(combineProgram1.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, head); // base
        glUniform1i(combineProgram1.GetQualfiterLoc("U_MainTexture"), 0);
        fsq.DrawToQuarter(combineProgram1.GetQualfiterLoc("pos"), combineProgram1.GetQualfiterLoc("texcoord"), 2);

        // 左下，反正片叠底
        glUseProgram(combineProgram2.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, head); // base
        glUniform1i(combineProgram2.GetQualfiterLoc("U_MainTexture"), 0);
        fsq.DrawToQuarter(combineProgram2.GetQualfiterLoc("pos"), combineProgram2.GetQualfiterLoc("texcoord"), 1);
        glBindTexture(GL_TEXTURE_2D, 0);

        // 右下，颜色减淡
        glUseProgram(combineProgram3.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, head); // base
        glUniform1i(combineProgram3.GetQualfiterLoc("U_BaseTexture"), 0);;
        fsq.DrawToQuarter(combineProgram3.GetQualfiterLoc("pos"), combineProgram3.GetQualfiterLoc("texcoord"), 3);
        glBindTexture(GL_TEXTURE_2D, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0); // 重置
        glFinish();
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

    glDisable(GL_BLEND);
    return 0;
}

int GLRenderer::Fog_Linear(HWND hwnd, HDC dc, int viewW, int viewH)
{
    GPUProgram originProgram;
    originProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fog/fog.vs"));
    originProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/fog/fog_linear.fs"));
    originProgram.Link();
    GL_CHECK_ERROR;

    FullScreenQuad fsq;
    fsq.Init();
    this->BlurInit(viewW, viewH);

    // model
    ObjModel cube1, cube2, cube3;
    cube1.InitModel(S_PATH("resource/model/Cube.obj"));
    cube2.InitModel(S_PATH("resource/model/Cube.obj"));
    cube3.InitModel(S_PATH("resource/model/Cube.obj"));
    // quad.InitModel(S_PATH("resource/model/Quad.obj"));
    // sphere.InitModel(S_PATH("resource/model/Sphere.obj"));

    // 传递参数到shader
    originProgram.DetectAttributes({"pos", "texcoord", "normal"});
    originProgram.DetectUniforms({"M", "V", "P", "NM", "U_LightPos",
                                    "U_DiffuseLightColor", "U_DiffuseMaterial",
                                    "U_AmbientLightColor", "U_AmbientMaterial",
                                    "U_SpecularLightColor", "U_SpecularMaterial",
                                    "U_EyePos", "U_SpotLightDirect", "U_CutOffAngle",
                                    "U_FogStart", "U_FogEnd", "U_FogColor"});
    GL_CHECK_ERROR;

    const float WH = static_cast<float>(viewW) / static_cast<float>(viewH);

    glm::mat4 cubeModel = glm::translate<float>(-3.0f, 0.0f, 4.0f)* glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    glm::mat4 normalMatrix = glm::inverseTranspose(cubeModel);

    glm::mat4 cubeModel2 = glm::translate<float>(-1.0f, 0.0f, 2.0f)* glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    glm::mat4 normalMatrix2 = glm::inverseTranspose(cubeModel2);

    glm::mat4 cubeModel3 = glm::translate<float>(1.0f, 0.0f, -6.0f) * glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    glm::mat4 normalMatrix3 = glm::inverseTranspose(cubeModel3);

    glm::mat4 projection = glm::perspective(50.0f, WH, 0.1f, 1000.0f);
    glm::mat4 viewMatrix1 = glm::lookAt(glm::vec3(1.0f, -1.0f, 10.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f));

    float ambientLightColor[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float ambientMaterial[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float diffuseLightColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float diffuseMaterial[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float specularLightColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float specularMaterial[] = {0.8f, 0.8f, 0.8f, 1.0f};
    float lightPos[] = {1.0f, 1.0f, 0.0f, 0.0f};
    float eyePos[] = {-1.0f, 2.0f, 10.0};
    float spotLightDirection[] = {0.0f, -1.0f, 0.0f, 128.0f};
    float spotLightCutOffAngle = 0.0; // degree

    // FOG
    float fogStart = 2.0f;
    float fogEnd = 18.0f;
    float fogColor[] = {0.1f, 0.4f, 0.7f, 1.0f};

    // opengl 环境设置
    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // 开启 alpha 混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    MSG msg;
    // glClearColor(0.1f, 0.4f, 0.7f, 1.0f);
    glClearColor(0.6f, 0.6f, 0.6f, 1.0f);

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
        // 编译命令
        // no blur
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 光照
        glUseProgram(originProgram.GetGPUProgram());
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("V"), 1, GL_FALSE, glm::value_ptr(viewMatrix1));    // V visual 视口
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影

        // ambient
        glUniform4fv(originProgram.GetQualfiterLoc("U_AmbientLightColor"), 1, ambientLightColor);
        glUniform4fv(originProgram.GetQualfiterLoc("U_AmbientMaterial"), 1, ambientMaterial);

        // diffuse
        glUniform4fv(originProgram.GetQualfiterLoc("U_DiffuseLightColor"), 1, diffuseLightColor);
        glUniform4fv(originProgram.GetQualfiterLoc("U_DiffuseMaterial"), 1, diffuseMaterial);
        GL_CHECK_ERROR;

        // specular
        glUniform4fv(originProgram.GetQualfiterLoc("U_SpecularLightColor"), 1, specularLightColor);
        glUniform4fv(originProgram.GetQualfiterLoc("U_SpecularMaterial"), 1, specularMaterial);
        glUniform3fv(originProgram.GetQualfiterLoc("U_EyePos"), 1, eyePos);
        GL_CHECK_ERROR;

        glUniform4fv(originProgram.GetQualfiterLoc("U_SpotLightDirect"), 1, spotLightDirection);

        // fog
        glUniform1f(originProgram.GetQualfiterLoc("U_FogStart"), fogStart);
        glUniform1f(originProgram.GetQualfiterLoc("U_FogEnd"), fogEnd);
        glUniform4fv(originProgram.GetQualfiterLoc("U_FogColor"), 1, fogColor);

        GL_CHECK_ERROR;

        // 聚光
        // lightPos[0] = -1.8f;
        // lightPos[1] = 3.0f;
        // lightPos[2] = -5.7f;
        // lightPos[3] = 0.0f;
        spotLightCutOffAngle = 0.0f; // 只用方向光
        glUniform1f(originProgram.GetQualfiterLoc("U_CutOffAngle"), spotLightCutOffAngle);
        glUniform4fv(originProgram.GetQualfiterLoc("U_LightPos"), 1, lightPos);
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(cubeModel)); // M model,模型视图移动，
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影
        GL_CHECK_ERROR;

        glUniformMatrix4fv(originProgram.GetQualfiterLoc("V"), 1, GL_FALSE, glm::value_ptr(viewMatrix1));    // V visual 视口
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(cubeModel)); // M model,模型视图移动，
        cube1.Bind(originProgram.GetQualfiterLoc("pos"), originProgram.GetQualfiterLoc("normal"));
        cube1.Draw();

        glUniformMatrix4fv(originProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(cubeModel2)); // M model,模型视图移动，
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix2));  // 投影
        cube2.Bind(originProgram.GetQualfiterLoc("pos"), originProgram.GetQualfiterLoc("normal"));
        cube2.Draw();

        glUniformMatrix4fv(originProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(cubeModel3)); // M model,模型视图移动，
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix3));  // 投影
        cube3.Bind(originProgram.GetQualfiterLoc("pos"), originProgram.GetQualfiterLoc("normal"));
        cube3.Draw();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0); // 重置
        glFinish();
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

    glDisable(GL_BLEND);
    return 0;
}

int GLRenderer::Fog_EXP(HWND hwnd, HDC dc, int viewW, int viewH)
{
    GPUProgram originProgram;
    originProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fog/fog.vs"));
    originProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/fog/fog_exp.fs"));
    originProgram.Link();
    GL_CHECK_ERROR;

    FullScreenQuad fsq;
    fsq.Init();
    this->BlurInit(viewW, viewH);

    // model
    ObjModel cube1, cube2, cube3;
    cube1.InitModel(S_PATH("resource/model/Cube.obj"));
    cube2.InitModel(S_PATH("resource/model/Cube.obj"));
    cube3.InitModel(S_PATH("resource/model/Cube.obj"));
    // quad.InitModel(S_PATH("resource/model/Quad.obj"));
    // sphere.InitModel(S_PATH("resource/model/Sphere.obj"));

    // 传递参数到shader
    originProgram.DetectAttributes({"pos", "texcoord", "normal"});
    originProgram.DetectUniforms({"M", "V", "P", "NM", "U_LightPos",
                                    "U_DiffuseLightColor", "U_DiffuseMaterial",
                                    "U_AmbientLightColor", "U_AmbientMaterial",
                                    "U_SpecularLightColor", "U_SpecularMaterial",
                                    "U_EyePos", "U_SpotLightDirect", "U_CutOffAngle",
                                    "U_FogStart", "U_FogEnd", "U_FogColor", "U_FogDensity"});
    GL_CHECK_ERROR;

    const float WH = static_cast<float>(viewW) / static_cast<float>(viewH);

    glm::mat4 cubeModel = glm::translate<float>(-3.0f, 0.0f, 4.0f)* glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    glm::mat4 normalMatrix = glm::inverseTranspose(cubeModel);

    glm::mat4 cubeModel2 = glm::translate<float>(-1.0f, 0.0f, 2.0f)* glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    glm::mat4 normalMatrix2 = glm::inverseTranspose(cubeModel2);

    glm::mat4 cubeModel3 = glm::translate<float>(1.0f, 0.0f, -6.0f) * glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    glm::mat4 normalMatrix3 = glm::inverseTranspose(cubeModel3);

    glm::mat4 projection = glm::perspective(50.0f, WH, 0.1f, 1000.0f);
    glm::mat4 viewMatrix1 = glm::lookAt(glm::vec3(1.0f, -1.0f, 10.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f));

    float ambientLightColor[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float ambientMaterial[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float diffuseLightColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float diffuseMaterial[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float specularLightColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float specularMaterial[] = {0.8f, 0.8f, 0.8f, 1.0f};
    float lightPos[] = {1.0f, 1.0f, 0.0f, 0.0f};
    float eyePos[] = {-1.0f, 2.0f, 10.0};
    float spotLightDirection[] = {0.0f, -1.0f, 0.0f, 128.0f};
    float spotLightCutOffAngle = 0.0; // degree

    // FOG
    float fogStart = 2.0f;
    float fogEnd = 18.0f;
    float fogColor[] = {0.6f, 0.6f, 0.6f, 1.0f};
    float fogDensity = 0.1f;

    // opengl 环境设置
    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // 开启 alpha 混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    MSG msg;
    // glClearColor(0.1f, 0.4f, 0.7f, 1.0f);
    glClearColor(0.6f, 0.6f, 0.6f, 1.0f);

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
        // 编译命令
        // no blur
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 光照
        glUseProgram(originProgram.GetGPUProgram());
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("V"), 1, GL_FALSE, glm::value_ptr(viewMatrix1));    // V visual 视口
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影

        // ambient
        glUniform4fv(originProgram.GetQualfiterLoc("U_AmbientLightColor"), 1, ambientLightColor);
        glUniform4fv(originProgram.GetQualfiterLoc("U_AmbientMaterial"), 1, ambientMaterial);

        // diffuse
        glUniform4fv(originProgram.GetQualfiterLoc("U_DiffuseLightColor"), 1, diffuseLightColor);
        glUniform4fv(originProgram.GetQualfiterLoc("U_DiffuseMaterial"), 1, diffuseMaterial);
        GL_CHECK_ERROR;

        // specular
        glUniform4fv(originProgram.GetQualfiterLoc("U_SpecularLightColor"), 1, specularLightColor);
        glUniform4fv(originProgram.GetQualfiterLoc("U_SpecularMaterial"), 1, specularMaterial);
        glUniform3fv(originProgram.GetQualfiterLoc("U_EyePos"), 1, eyePos);
        GL_CHECK_ERROR;

        glUniform4fv(originProgram.GetQualfiterLoc("U_SpotLightDirect"), 1, spotLightDirection);

        // fog
        glUniform1f(originProgram.GetQualfiterLoc("U_FogStart"), fogStart);
        glUniform1f(originProgram.GetQualfiterLoc("U_FogEnd"), fogEnd);
        glUniform4fv(originProgram.GetQualfiterLoc("U_FogColor"), 1, fogColor);
        glUniform1f(originProgram.GetQualfiterLoc("U_FogDensity"), fogDensity);

        GL_CHECK_ERROR;

        // 聚光
        // lightPos[0] = -1.8f;
        // lightPos[1] = 3.0f;
        // lightPos[2] = -5.7f;
        // lightPos[3] = 0.0f;
        spotLightCutOffAngle = 0.0f; // 只用方向光
        glUniform1f(originProgram.GetQualfiterLoc("U_CutOffAngle"), spotLightCutOffAngle);
        glUniform4fv(originProgram.GetQualfiterLoc("U_LightPos"), 1, lightPos);
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(cubeModel)); // M model,模型视图移动，
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影
        GL_CHECK_ERROR;

        glUniformMatrix4fv(originProgram.GetQualfiterLoc("V"), 1, GL_FALSE, glm::value_ptr(viewMatrix1));    // V visual 视口
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(cubeModel)); // M model,模型视图移动，
        cube1.Bind(originProgram.GetQualfiterLoc("pos"), originProgram.GetQualfiterLoc("normal"));
        cube1.Draw();

        glUniformMatrix4fv(originProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(cubeModel2)); // M model,模型视图移动，
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix2));  // 投影
        cube2.Bind(originProgram.GetQualfiterLoc("pos"), originProgram.GetQualfiterLoc("normal"));
        cube2.Draw();

        glUniformMatrix4fv(originProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(cubeModel3)); // M model,模型视图移动，
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix3));  // 投影
        cube3.Bind(originProgram.GetQualfiterLoc("pos"), originProgram.GetQualfiterLoc("normal"));
        cube3.Draw();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0); // 重置
        glFinish();
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

    glDisable(GL_BLEND);
    return 0;
}

int GLRenderer::Fog_EXPX(HWND hwnd, HDC dc, int viewW, int viewH)
{
    GPUProgram originProgram;
    originProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fog/fog.vs"));
    originProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/fog/fog_expx.fs"));
    originProgram.Link();
    GL_CHECK_ERROR;

    FullScreenQuad fsq;
    fsq.Init();
    this->BlurInit(viewW, viewH);

    // model
    ObjModel cube1, cube2, cube3;
    cube1.InitModel(S_PATH("resource/model/Cube.obj"));
    cube2.InitModel(S_PATH("resource/model/Cube.obj"));
    cube3.InitModel(S_PATH("resource/model/Cube.obj"));
    // quad.InitModel(S_PATH("resource/model/Quad.obj"));
    // sphere.InitModel(S_PATH("resource/model/Sphere.obj"));

    // 传递参数到shader
    originProgram.DetectAttributes({"pos", "texcoord", "normal"});
    originProgram.DetectUniforms({"M", "V", "P", "NM", "U_LightPos",
                                    "U_DiffuseLightColor", "U_DiffuseMaterial",
                                    "U_AmbientLightColor", "U_AmbientMaterial",
                                    "U_SpecularLightColor", "U_SpecularMaterial",
                                    "U_EyePos", "U_SpotLightDirect", "U_CutOffAngle",
                                    "U_FogStart", "U_FogEnd", "U_FogColor", "U_FogDensity", "U_FogGradient"});
    GL_CHECK_ERROR;

    const float WH = static_cast<float>(viewW) / static_cast<float>(viewH);

    glm::mat4 cubeModel = glm::translate<float>(-3.0f, 0.0f, 4.0f)* glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    glm::mat4 normalMatrix = glm::inverseTranspose(cubeModel);

    glm::mat4 cubeModel2 = glm::translate<float>(-1.0f, 0.0f, 2.0f)* glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    glm::mat4 normalMatrix2 = glm::inverseTranspose(cubeModel2);

    glm::mat4 cubeModel3 = glm::translate<float>(1.0f, 0.0f, -6.0f) * glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    glm::mat4 normalMatrix3 = glm::inverseTranspose(cubeModel3);

    glm::mat4 projection = glm::perspective(50.0f, WH, 0.1f, 1000.0f);
    glm::mat4 viewMatrix1 = glm::lookAt(glm::vec3(1.0f, -1.0f, 10.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f));

    float ambientLightColor[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float ambientMaterial[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float diffuseLightColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float diffuseMaterial[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float specularLightColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float specularMaterial[] = {0.8f, 0.8f, 0.8f, 1.0f};
    float lightPos[] = {1.0f, 1.0f, 0.0f, 0.0f};
    float eyePos[] = {-1.0f, 2.0f, 10.0};
    float spotLightDirection[] = {0.0f, -1.0f, 0.0f, 128.0f};
    float spotLightCutOffAngle = 0.0; // degree

    // FOG
    float fogStart = 2.0f;
    float fogEnd = 18.0f;
    float fogColor[] = {0.6f, 0.6f, 0.6f, 1.0f};
    float fogDensity = 0.1f;
    float fogGradient = 2.0f; // OpenGL 固定管线中指定的就是 2


    // opengl 环境设置
    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // 开启 alpha 混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    MSG msg;
    // glClearColor(0.1f, 0.4f, 0.7f, 1.0f);
    glClearColor(0.6f, 0.6f, 0.6f, 1.0f);

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
        // 编译命令
        // no blur
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 光照
        glUseProgram(originProgram.GetGPUProgram());
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("V"), 1, GL_FALSE, glm::value_ptr(viewMatrix1));    // V visual 视口
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影

        // ambient
        glUniform4fv(originProgram.GetQualfiterLoc("U_AmbientLightColor"), 1, ambientLightColor);
        glUniform4fv(originProgram.GetQualfiterLoc("U_AmbientMaterial"), 1, ambientMaterial);

        // diffuse
        glUniform4fv(originProgram.GetQualfiterLoc("U_DiffuseLightColor"), 1, diffuseLightColor);
        glUniform4fv(originProgram.GetQualfiterLoc("U_DiffuseMaterial"), 1, diffuseMaterial);
        GL_CHECK_ERROR;

        // specular
        glUniform4fv(originProgram.GetQualfiterLoc("U_SpecularLightColor"), 1, specularLightColor);
        glUniform4fv(originProgram.GetQualfiterLoc("U_SpecularMaterial"), 1, specularMaterial);
        glUniform3fv(originProgram.GetQualfiterLoc("U_EyePos"), 1, eyePos);
        GL_CHECK_ERROR;

        glUniform4fv(originProgram.GetQualfiterLoc("U_SpotLightDirect"), 1, spotLightDirection);

        // fog
        glUniform1f(originProgram.GetQualfiterLoc("U_FogStart"), fogStart);
        glUniform1f(originProgram.GetQualfiterLoc("U_FogEnd"), fogEnd);
        glUniform4fv(originProgram.GetQualfiterLoc("U_FogColor"), 1, fogColor);
        glUniform1f(originProgram.GetQualfiterLoc("U_FogDensity"), fogDensity);
        glUniform1f(originProgram.GetQualfiterLoc("U_FogGradient"), fogGradient);

        GL_CHECK_ERROR;

        // 聚光
        // lightPos[0] = -1.8f;
        // lightPos[1] = 3.0f;
        // lightPos[2] = -5.7f;
        // lightPos[3] = 0.0f;
        spotLightCutOffAngle = 0.0f; // 只用方向光
        glUniform1f(originProgram.GetQualfiterLoc("U_CutOffAngle"), spotLightCutOffAngle);
        glUniform4fv(originProgram.GetQualfiterLoc("U_LightPos"), 1, lightPos);
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(cubeModel)); // M model,模型视图移动，
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影
        GL_CHECK_ERROR;

        glUniformMatrix4fv(originProgram.GetQualfiterLoc("V"), 1, GL_FALSE, glm::value_ptr(viewMatrix1));    // V visual 视口
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(cubeModel)); // M model,模型视图移动，
        cube1.Bind(originProgram.GetQualfiterLoc("pos"), originProgram.GetQualfiterLoc("normal"));
        cube1.Draw();

        glUniformMatrix4fv(originProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(cubeModel2)); // M model,模型视图移动，
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix2));  // 投影
        cube2.Bind(originProgram.GetQualfiterLoc("pos"), originProgram.GetQualfiterLoc("normal"));
        cube2.Draw();

        glUniformMatrix4fv(originProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(cubeModel3)); // M model,模型视图移动，
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix3));  // 投影
        cube3.Bind(originProgram.GetQualfiterLoc("pos"), originProgram.GetQualfiterLoc("normal"));
        cube3.Draw();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0); // 重置
        glFinish();
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

    glDisable(GL_BLEND);
    return 0;
}


int GLRenderer::Skybox(HWND hwnd, HDC dc, int viewW, int viewH)
{
    GPUProgram originProgram;
    originProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/skybox.vs"));
    originProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/skybox.fs"));
    originProgram.Link();

    // 传递参数到shader
    originProgram.DetectAttributes({"pos"});
    originProgram.DetectUniforms({"M", "V", "P", "U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram sphereProgram;
    sphereProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/test/sphere.vs"));
    sphereProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/test/sphere.fs"));
    sphereProgram.Link();
    sphereProgram.DetectAttributes({"pos"});
    sphereProgram.DetectUniforms({"M", "V", "P"});
    GL_CHECK_ERROR;

    FullScreenQuad fsq;
    fsq.Init();
    this->BlurInit(viewW, viewH);

    // model
    ObjModel cube1, sphere;
    cube1.InitModel(S_PATH("resource/model/Cube.obj"));
    sphere.InitModel(S_PATH("resource/model/Sphere.obj"));

    const float WH = static_cast<float>(viewW) / static_cast<float>(viewH);

    glm::mat4 cubeModel ;//= glm::translate<float>(-3.0f, 0.0f, 4.0f)* glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    glm::mat4 normalMatrix = glm::inverseTranspose(cubeModel);
    glm::mat4 projection = glm::perspective(50.0f, WH, 0.1f, 1000.0f);
    // glm::mat4 viewMatrix1 = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),
    //                                     glm::vec3(0.0f, 1.0f, 0.0f),
    //                                     glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 sphereModel = glm::translate<float>(0.0f, 0.0f, -4.0f) * glm::scale<float>(0.5f,0.5f,0.5f);
    glm::mat4 sphereNormalMatrix = glm::inverseTranspose(sphereModel);

    glm::mat4 sphereViewMatrix1 = glm::lookAt(glm::vec3(1.0f, 0.0f, 0.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f));

    GLuint mainTexture = SOIL_load_OGL_cubemap("resource/image/skybox/right.png", "resource/image/skybox/left.png",
                                                "resource/image/skybox/top.png", "resource/image/skybox/bottom.png",
                                                "resource/image/skybox/back.png", "resource/image/skybox/front.png",
                                                0, 0, SOIL_FLAG_POWER_OF_TWO);

    // opengl 环境设置
    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // 开启 alpha 混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    MSG msg;
    // glClearColor(0.1f, 0.4f, 0.7f, 1.0f);
    glClearColor(0.6f, 0.6f, 0.6f, 1.0f);

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
        // 编译命令
        // no blur
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);
        glUseProgram(originProgram.GetGPUProgram());
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("V"), 1, GL_FALSE, identity);    // V visual 视口
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(cubeModel)); // M model,模型视图移动

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mainTexture); // base

        glUniform1i(originProgram.GetQualfiterLoc("U_MainTexture"), 0);
        GL_CHECK_ERROR;

        cube1.Bind(originProgram.GetQualfiterLoc("pos")); //, originProgram.GetQualfiterLoc("texcoord"), originProgram.GetQualfiterLoc("normal"));
        cube1.Draw();
        glEnable(GL_DEPTH_TEST);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Test
        glUseProgram(sphereProgram.GetGPUProgram());
        glUniformMatrix4fv(sphereProgram.GetQualfiterLoc("V"), 1, GL_FALSE, identity);    // V visual 视口
        glUniformMatrix4fv(sphereProgram.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        glUniformMatrix4fv(sphereProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(sphereModel)); // M model,模型视图移动
        sphere.Bind(sphereProgram.GetQualfiterLoc("pos"));
        sphere.Draw();
        GL_CHECK_ERROR;

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0); // 重置
        glFinish();
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

    glDisable(GL_BLEND);
    return 0;
}

int GLRenderer::SkyboxReflection(HWND hwnd, HDC dc, int viewW, int viewH)
{
    GPUProgram originProgram;
    originProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/sky/skybox.vs"));
    originProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/sky/skybox.fs"));
    originProgram.Link();

    // 传递参数到shader
    originProgram.DetectAttributes({"pos"});
    originProgram.DetectUniforms({"M", "V", "P", "U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram reflectionProgram;
    reflectionProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/sky/reflection.vs"));
    reflectionProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/sky/reflection.fs"));
    reflectionProgram.Link();
    reflectionProgram.DetectAttributes({"pos", "normal"});
    reflectionProgram.DetectUniforms({"M", "V", "P", "NM", "U_MainTexture"});
    GL_CHECK_ERROR;

    FullScreenQuad fsq;
    fsq.Init();
    this->BlurInit(viewW, viewH);

    // model
    ObjModel cube1, sphere;
    cube1.InitModel(S_PATH("resource/model/Cube.obj"));
    sphere.InitModel(S_PATH("resource/model/Sphere.obj"));

    const float WH = static_cast<float>(viewW) / static_cast<float>(viewH);

    glm::mat4 cubeModel; // = glm::translate<float>(-3.0f, 0.0f, 4.0f)* glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    glm::mat4 normalMatrix = glm::inverseTranspose(cubeModel);
    glm::mat4 projection = glm::perspective(50.0f, WH, 0.1f, 1000.0f);
    // glm::mat4 viewMatrix1 = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),
    //                                     glm::vec3(0.0f, 1.0f, 0.0f),
    //                                     glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 sphereModel = glm::translate<float>(0.0f, 0.0f, -4.0f);// * glm::scale<float>(0.1f,0.1f,0.1f);
    glm::mat4 sphereNormalMatrix = glm::inverseTranspose(sphereModel);

    glm::mat4 sphereViewMatrix1 = glm::lookAt(glm::vec3(1.0f, 0.0f, 0.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f));

    GLuint mainTexture = SOIL_load_OGL_cubemap(S_PATH("resource/image/skybox/right.png"), S_PATH("resource/image/skybox/left.png"),
                                                S_PATH("resource/image/skybox/top.png"), S_PATH("resource/image/skybox/bottom.png"),
                                                S_PATH("resource/image/skybox/back.png"), S_PATH("resource/image/skybox/front.png"),
                                                0, 0, SOIL_FLAG_POWER_OF_TWO);

    // opengl 环境设置
    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // 开启 alpha 混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    MSG msg;
    // glClearColor(0.1f, 0.4f, 0.7f, 1.0f);
    glClearColor(0.6f, 0.6f, 0.6f, 1.0f);

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
        // 编译命令
        // no blur
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);
        glUseProgram(originProgram.GetGPUProgram());
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("V"), 1, GL_FALSE, identity);    // V visual 视口
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(cubeModel)); // M model,模型视图移动

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mainTexture); // base
        glUniform1i(originProgram.GetQualfiterLoc("U_MainTexture"), 0);
        cube1.Bind(originProgram.GetQualfiterLoc("pos")); //, originProgram.GetQualfiterLoc("texcoord"), originProgram.GetQualfiterLoc("normal"));
        cube1.Draw();
        glEnable(GL_DEPTH_TEST);
        glBindTexture(GL_TEXTURE_2D, 0);
        GL_CHECK_ERROR;

        // Test
        glUseProgram(reflectionProgram.GetGPUProgram());
        glUniformMatrix4fv(reflectionProgram.GetQualfiterLoc("V"), 1, GL_FALSE, identity);    // V visual 视口
        glUniformMatrix4fv(reflectionProgram.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        glUniformMatrix4fv(reflectionProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(sphereModel)); // M model,模型视图移动
        glUniformMatrix4fv(reflectionProgram.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(sphereNormalMatrix));
        glBindTexture(GL_TEXTURE_CUBE_MAP, mainTexture); // base
        glUniform1i(reflectionProgram.GetQualfiterLoc("U_MainTexture"), 0);
        sphere.Bind(reflectionProgram.GetQualfiterLoc("pos"), reflectionProgram.GetQualfiterLoc("normal"));
        sphere.Draw();
        GL_CHECK_ERROR;

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0); // 重置
        glFinish();
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

    glDisable(GL_BLEND);
    return 0;
}

int GLRenderer::SkyboxReflectionTest(HWND hwnd, HDC dc, int viewW, int viewH)
{
	GPUProgram originalProgram;
	originalProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/sky/skybox.vs"));
	originalProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/sky/skybox.fs"));
	originalProgram.Link();
	originalProgram.DetectAttributes({"pos", "texcoord", "normal"});
	originalProgram.DetectUniforms({"M", "V", "P", "U_MainTexture"});

	GPUProgram reflectionProgram;
	reflectionProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/sky/reflection.vs"));
	reflectionProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/sky/reflection.fs"));
	reflectionProgram.Link();
	reflectionProgram.DetectAttributes({"pos", "texcoord", "normal"});
	reflectionProgram.DetectUniforms({"M", "V", "P", "NM", "U_MainTexture"});

	//init 3d model
	ObjModel cube,sphere;
	cube.InitModel(S_PATH("resource/model/Cube.obj"));
	sphere.InitModel(S_PATH("resource/model/Sphere.obj"));

	float identity[] = {
		1.0f,0,0,0,
		0,1.0f,0,0,
		0,0,1.0f,0,
		0,0,0,1.0f
	};

	glm::mat4 cubeModel;
	glm::mat4 sphereModel = glm::translate(0.0f, 0.0f, -4.0f);
	glm::mat4 sphereNormalMatrix = glm::inverseTranspose(sphereModel);
	glm::mat4 cubeNormalMatrix = glm::inverseTranspose(cubeModel);

    glm::mat4 projectionMatrix = glm::perspective(50.0f, (float)viewW / (float)viewH, 0.1f, 1000.0f);
    glClearColor(0.6f, 0.6f, 0.6f, 1.0f);

    GLuint mainTexture = SOIL_load_OGL_cubemap("resource/image/skybox/right.png", "resource/image/skybox/left.png",
                                               "resource/image/skybox/top.png", "resource/image/skybox/bottom.png",
                                               "resource/image/skybox/back.png", "resource/image/skybox/front.png",
                                               0, 0, SOIL_FLAG_POWER_OF_TWO);
    ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	MSG msg;
	while (true)
	{
		if (PeekMessage(&msg,NULL,NULL,NULL,PM_REMOVE))
		{
			if (msg.message==WM_QUIT)
			{
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		//normal rgba
		glUseProgram(originalProgram.GetGPUProgram());
		glUniformMatrix4fv(originalProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(cubeModel));
		glUniformMatrix4fv(originalProgram.GetQualfiterLoc("V"), 1, GL_FALSE, identity);
		glUniformMatrix4fv(originalProgram.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));

		glBindTexture(GL_TEXTURE_CUBE_MAP, mainTexture);
		glUniform1i(originalProgram.GetQualfiterLoc("U_MainTexture"),0);
		cube.Bind(originalProgram.GetQualfiterLoc("pos"), originalProgram.GetQualfiterLoc("texcoord"), originalProgram.GetQualfiterLoc("normal"));
		cube.Draw();
		glEnable(GL_DEPTH_TEST);
		//draw sphere
		glUseProgram(reflectionProgram.GetGPUProgram());
		glUniformMatrix4fv(reflectionProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(sphereModel));
		glUniformMatrix4fv(reflectionProgram.GetQualfiterLoc("V"), 1, GL_FALSE, identity);
		glUniformMatrix4fv(reflectionProgram.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
		glUniformMatrix4fv(reflectionProgram.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(sphereNormalMatrix));

		glBindTexture(GL_TEXTURE_CUBE_MAP, mainTexture);
		glUniform1i(reflectionProgram.GetQualfiterLoc("U_MainTexture"), 0);
		sphere.Bind(reflectionProgram.GetQualfiterLoc("pos"), reflectionProgram.GetQualfiterLoc("texcoord"), reflectionProgram.GetQualfiterLoc("normal"));
		sphere.Draw();

		glFlush();
		SwapBuffers(dc);
	}
	return 0;
}

int GLRenderer::SkyboxRefraction(HWND hwnd, HDC dc, int viewW, int viewH)
{
    GPUProgram originProgram;
    originProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/sky/skybox.vs"));
    originProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/sky/skybox.fs"));
    originProgram.Link();

    // 传递参数到shader
    originProgram.DetectAttributes({"pos"});
    originProgram.DetectUniforms({"M", "V", "P", "U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram reflectionProgram;
    reflectionProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/sky/reflection.vs"));
    reflectionProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/sky/reflection.fs"));
    reflectionProgram.Link();
    reflectionProgram.DetectAttributes({"pos", "normal"});
    reflectionProgram.DetectUniforms({"M", "V", "P", "NM", "U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram refractionProgram;
    refractionProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/sky/refraction.vs"));
    refractionProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/sky/refraction.fs"));
    refractionProgram.Link();
    refractionProgram.DetectAttributes({"pos", "normal"});
    refractionProgram.DetectUniforms({"M", "V", "P", "NM", "U_MainTexture"});
    GL_CHECK_ERROR;

    FullScreenQuad fsq;
    fsq.Init();
    this->BlurInit(viewW, viewH);

    // model
    ObjModel cube1, sphereReflect, sphereRefract;
    cube1.InitModel(S_PATH("resource/model/Cube.obj"));
    sphereReflect.InitModel(S_PATH("resource/model/Sphere.obj"));
    sphereRefract.InitModel(S_PATH("resource/model/Sphere.obj"));

    const float WH = static_cast<float>(viewW) / static_cast<float>(viewH);

    glm::mat4 cubeModel; // = glm::translate<float>(-3.0f, 0.0f, 4.0f)* glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    glm::mat4 normalMatrix = glm::inverseTranspose(cubeModel);
    glm::mat4 projection = glm::perspective(50.0f, WH, 0.1f, 1000.0f);
    // glm::mat4 viewMatrix1 = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),
    //                                     glm::vec3(0.0f, 1.0f, 0.0f),
    //                                     glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 sphereModelReflect = glm::translate<float>(-1.5f, 0.0f, -4.0f);// * glm::scale<float>(0.1f,0.1f,0.1f);
    glm::mat4 sphereNormalMatrixA = glm::inverseTranspose(sphereModelReflect);

    glm::mat4 sphereModelRefract = glm::translate<float>(1.5f, 0.0f, -4.0f);
    glm::mat4 sphereNormalMatrixB = glm::inverseTranspose(sphereModelRefract);

    GLuint mainTexture = SOIL_load_OGL_cubemap(S_PATH("resource/image/skybox/right.png"), S_PATH("resource/image/skybox/left.png"),
                                                S_PATH("resource/image/skybox/top.png"), S_PATH("resource/image/skybox/bottom.png"),
                                                S_PATH("resource/image/skybox/back.png"), S_PATH("resource/image/skybox/front.png"),
                                                0, 0, SOIL_FLAG_POWER_OF_TWO);

    // opengl 环境设置
    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // 开启 alpha 混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    MSG msg;
    // glClearColor(0.1f, 0.4f, 0.7f, 1.0f);
    glClearColor(0.6f, 0.6f, 0.6f, 1.0f);

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
        // 编译命令
        // no blur
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);
        glUseProgram(originProgram.GetGPUProgram());
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("V"), 1, GL_FALSE, identity);    // V visual 视口
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        glUniformMatrix4fv(originProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(cubeModel)); // M model,模型视图移动

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mainTexture); // base
        glUniform1i(originProgram.GetQualfiterLoc("U_MainTexture"), 0);
        cube1.Bind(originProgram.GetQualfiterLoc("pos")); //, originProgram.GetQualfiterLoc("texcoord"), originProgram.GetQualfiterLoc("normal"));
        cube1.Draw();
        glEnable(GL_DEPTH_TEST);
        glBindTexture(GL_TEXTURE_2D, 0);
        GL_CHECK_ERROR;

        // Test
        glUseProgram(reflectionProgram.GetGPUProgram());
        glUniformMatrix4fv(reflectionProgram.GetQualfiterLoc("V"), 1, GL_FALSE, identity);    // V visual 视口
        glUniformMatrix4fv(reflectionProgram.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        glUniformMatrix4fv(reflectionProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(sphereModelReflect)); // M model,模型视图移动
        glUniformMatrix4fv(reflectionProgram.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(sphereNormalMatrixA));
        glBindTexture(GL_TEXTURE_CUBE_MAP, mainTexture); // base
        glUniform1i(reflectionProgram.GetQualfiterLoc("U_MainTexture"), 0);
        sphereReflect.Bind(reflectionProgram.GetQualfiterLoc("pos"), reflectionProgram.GetQualfiterLoc("normal"));
        sphereReflect.Draw();
        GL_CHECK_ERROR;

        glUseProgram(refractionProgram.GetGPUProgram());
        glUniformMatrix4fv(refractionProgram.GetQualfiterLoc("V"), 1, GL_FALSE, identity);    // V visual 视口
        glUniformMatrix4fv(refractionProgram.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        glUniformMatrix4fv(refractionProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(sphereModelRefract)); // M model,模型视图移动
        glUniformMatrix4fv(refractionProgram.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(sphereNormalMatrixB));
        glBindTexture(GL_TEXTURE_CUBE_MAP, mainTexture); // base
        glUniform1i(refractionProgram.GetQualfiterLoc("U_MainTexture"), 0);
        sphereReflect.Bind(refractionProgram.GetQualfiterLoc("pos"), refractionProgram.GetQualfiterLoc("normal"));
        sphereReflect.Draw();
        GL_CHECK_ERROR;

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0); // 重置
        glFinish();
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

    glDisable(GL_BLEND);
    return 0;
}

int GLRenderer::ShadowTest1(HWND hwnd, HDC dc, int viewW, int viewH)
{
    // init
    // 0.get Program
    GPUProgram gpuProgSpecular;
    gpuProgSpecular.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/hdrLight.vs"));
    gpuProgSpecular.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/hdrLight.fs"));
    gpuProgSpecular.Link();
    GL_CHECK_ERROR;

    GPUProgram originProgram;
    originProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    originProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/fullscreenQuad.fs"));
    originProgram.Link();
    originProgram.DetectAttributes({"pos", "texcoord"});
    originProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram hdrProgram;
    hdrProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    hdrProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/hdrRenderer.fs"));
    hdrProgram.Link();
    hdrProgram.DetectAttributes({"pos", "texcoord"});
    hdrProgram.DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    GPUProgram combineProgram;
    combineProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    combineProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/combineHDRAndNormal.fs"));
    combineProgram.Link();
    combineProgram.DetectAttributes({"pos", "texcoord"});
    combineProgram.DetectUniforms({"U_MainTexture","U_HDRTexture"});
    GL_CHECK_ERROR;

    GPUProgram bloomProgram;
    bloomProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/light_bloom.vs"));
    bloomProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/light_bloom.fs"));
    bloomProgram.Link();
    bloomProgram.DetectAttributes({"pos"});
    bloomProgram.DetectUniforms({"M", "V", "P"});
    GL_CHECK_ERROR;

    GPUProgram depthProgram; // 只需要知道几何信息即可, 谁在前谁在后
    depthProgram.AttachShader(GL_VERTEX_SHADER, S_PATH("shader/shadow/simpleDepthBuffer.vs"));
    depthProgram.AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/shadow/simpleDepthBuffer.fs"));
    depthProgram.Link();
    depthProgram.DetectAttributes({"pos"});
    depthProgram.DetectUniforms({"M", "V", "P"});
    GL_CHECK_ERROR;

    FullScreenQuad fsq;
    fsq.Init();
    this->BlurInit(viewW, viewH);

    FBO fboDirectionLight;
    // sRGBA: r 8bit 0~255(0~1.0)
    fboDirectionLight.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboDirectionLight.AttachDepthBuffer("depth", viewW, viewH);
    fboDirectionLight.Finish();


    FBO fboHDR;
    fboHDR.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    fboHDR.AttachColorBuffer("hdrBuffer", GL_COLOR_ATTACHMENT1, GL_RGBA16F, viewW, viewH);
    fboHDR.AttachDepthBuffer("depth", viewW, viewH);
    fboHDR.Finish();
    GL_CHECK_ERROR

    // model
    ObjModel cube, quad, sphere;
    cube.InitModel(S_PATH("resource/model/Cube.obj"));
    quad.InitModel(S_PATH("resource/model/Quad.obj"));
    sphere.InitModel(S_PATH("resource/model/Sphere.obj"));

    // 传递参数到shader
    gpuProgSpecular.DetectAttributes({"pos", "texcoord", "normal"});
    gpuProgSpecular.DetectUniforms({"M", "V", "P", "NM", "U_LightPos",
                                    "U_DiffuseLightColor", "U_DiffuseMaterial",
                                    "U_AmbientLightColor", "U_AmbientMaterial",
                                    "U_SpecularLightColor", "U_SpecularMaterial",
                                    "U_EyePos", "U_SpotLightDirect", "U_CutOffAngle"});
    GL_CHECK_ERROR;

    glm::mat4 modelA = glm::translate<float>(6.0f, 0.0f, -6.0f) * glm::rotate<float>(-30.0f, 1.0f, 1.0f, 1.0f);
    const float WH = static_cast<float>(viewW) / static_cast<float>(viewH);
    glm::mat4 projection = glm::perspective(50.0f, WH, 0.1f, 1000.0f);
    glm::mat4 normalMatrix = glm::inverseTranspose(modelA);
    glm::mat4 viewMatrix1 = glm::lookAt(glm::vec3(14.0, 2.5f, -10.0f),
                                        glm::vec3(6.0f, 1.0f, -6.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f));

    // 求光线的相关矩阵，方便后续求深度图
    glm::mat4 lightViewMatrix = glm::lookAt(glm::vec3(6.0, 10.0f, -6.0f),
                                        glm::vec3(6.0f, 0.0f, -6.0f),
                                        glm::vec3(0.0f, 0.0f, -1.0f));
    // 正射投影
    glm::mat4 lightProjectMatrix = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 20.0f);


    glm::mat4 quadModel = glm::translate<float>(6.0f, -1.5f, -6.0f) * glm::rotate<float>(-90.0f, 1.0f, 0.0f, 0.0f)
        * glm::scale<float>(4.0f,4.0f,4.0f);
    glm::mat4 quadNormalMat = glm::inverseTranspose(quadModel);

    glm::mat4 sphereModel = glm::translate<float>(6.3f, 3.0f, -5.7f)*glm::scale<float>(0.2f,0.2f,0.2f);
    glm::mat4 sphereNormalMat = glm::inverseTranspose(sphereModel);

    // 第一帧拿到 depthFBU 上，这样就能拿到深度图
    FBO depthFBO;
    depthFBO.AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    depthFBO.AttachDepthBuffer("depth", viewW, viewH);
    depthFBO.Finish();

    float ambientLightColor[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float ambientMaterial[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float diffuseLightColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float diffuseMaterial[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float specularLightColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float specularMaterial[] = {0.8f, 0.8f, 0.8f, 1.0f};
    float lightPos[] = {0.0f, 1.5f, 0.0f, 0.0f};
    float eyePos[] = {0.0f, 0.0f, 0.0f};
    float spotLightDirection[] = {0.0f, -1.0f, 0.0f, 128.0f};
    float spotLightCutOffAngle = 0.0; // degree

    // opengl 环境设置
    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // 开启 alpha 混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    MSG msg;
    // glClearColor(0.1f, 0.4f, 0.7f, 1.0f);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

#if 1 // 拿取第一帧信息
    depthFBO.Bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(depthProgram.GetGPUProgram());
    glUniformMatrix4fv(depthProgram.GetQualfiterLoc("V"), 1, GL_FALSE, glm::value_ptr(lightViewMatrix));
    glUniformMatrix4fv(depthProgram.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(lightProjectMatrix));
    GL_CHECK_ERROR;

    glUniformMatrix4fv(depthProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(modelA));
    cube.Bind(depthProgram.GetQualfiterLoc("pos"));
    cube.Draw();

    glUniformMatrix4fv(depthProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(quadModel)); // M model,模型视图移动，
    quad.Bind(depthProgram.GetQualfiterLoc("pos"));
    quad.Draw();

    depthFBO.UnBind();
#endif

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
        // 编译命令
        glUseProgram(gpuProgSpecular.GetGPUProgram());
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("V"), 1, GL_FALSE, glm::value_ptr(viewMatrix1));    // V visual 视口
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影

        // ambient
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_AmbientLightColor"), 1, ambientLightColor);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_AmbientMaterial"), 1, ambientMaterial);

        // diffuse
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_DiffuseLightColor"), 1, diffuseLightColor);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_DiffuseMaterial"), 1, diffuseMaterial);
        GL_CHECK_ERROR;

        // specular
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_SpecularLightColor"), 1, specularLightColor);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_SpecularMaterial"), 1, specularMaterial);
        glUniform3fv(gpuProgSpecular.GetQualfiterLoc("U_EyePos"), 1, eyePos);
        GL_CHECK_ERROR;

        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_SpotLightDirect"), 1, spotLightDirection);
        GL_CHECK_ERROR;

        // 聚光
        lightPos[0] = -1.8f;
        lightPos[1] = 3.0f;
        lightPos[2] = -5.7f;
        lightPos[3] = 0.0f;
        spotLightCutOffAngle = 0.0f;
        glUniform1f(gpuProgSpecular.GetQualfiterLoc("U_CutOffAngle"), spotLightCutOffAngle);
        glUniform4fv(gpuProgSpecular.GetQualfiterLoc("U_LightPos"), 1, lightPos);
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(modelA)); // M model,模型视图移动，
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(normalMatrix));  // 投影
        GL_CHECK_ERROR;
        fboHDR.Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        cube.Bind(gpuProgSpecular.GetQualfiterLoc("pos"), gpuProgSpecular.GetQualfiterLoc("normal"));
        cube.Draw();

        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(quadModel)); // M model,模型视图移动，
        glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(quadNormalMat));  // 投影
        quad.Bind(gpuProgSpecular.GetQualfiterLoc("pos"), gpuProgSpecular.GetQualfiterLoc("normal"));
        quad.Draw();

        // glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(sphereModel)); // M model,模型视图移动，
        // glUniformMatrix4fv(gpuProgSpecular.GetQualfiterLoc("NM"), 1, GL_FALSE, glm::value_ptr(sphereNormalMat));  // 投影
        // sphere.Bind(gpuProgSpecular.GetQualfiterLoc("pos"), gpuProgSpecular.GetQualfiterLoc("normal"));


        glUseProgram(bloomProgram.GetGPUProgram());
        glUniformMatrix4fv(bloomProgram.GetQualfiterLoc("V"), 1, GL_FALSE, glm::value_ptr(viewMatrix1));    // V visual 视口
        glUniformMatrix4fv(bloomProgram.GetQualfiterLoc("P"), 1, GL_FALSE, glm::value_ptr(projection));     // 投影
        glUniformMatrix4fv(bloomProgram.GetQualfiterLoc("M"), 1, GL_FALSE, glm::value_ptr(sphereModel)); // M model,模型视图移动，
        sphere.Bind(bloomProgram.GetQualfiterLoc("pos"));
        sphere.Draw();

        fboHDR.UnBind();
        GL_CHECK_ERROR;
        glUseProgram(0); // 重置

        // no blur
        // glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(originProgram.GetGPUProgram());
        glBindTexture(GL_TEXTURE_2D, depthFBO.GetBuffer("color"));
        glUniform1i(originProgram.GetQualfiterLoc("U_MainTexture"), 0);
        fsq.DrawToQuarter(originProgram.GetQualfiterLoc("pos"), originProgram.GetQualfiterLoc("texcoord"), 2); // 右上
        glBindTexture(GL_TEXTURE_2D, 0);

        // 右上，HDR 图
        //glUseProgram(hdrProgram.GetGPUProgram());
        //glBindTexture(GL_TEXTURE_2D, fboHDR.GetBuffer("hdrBuffer"));
        //glUniform1i(hdrProgram.GetQualfiterLoc("U_MainTexture"), 0);
        //fsq.DrawToQuarter(hdrProgram.GetQualfiterLoc("pos"), hdrProgram.GetQualfiterLoc("texcoord"), 2);
        //glBindTexture(GL_TEXTURE_2D, 0);

        // 左下, HDR 高斯模糊图
        // 高斯模糊
        GLuint blurRes = this->Blur(fsq, fboHDR.GetBuffer("hdrBuffer"), 10);

        // 合成图
        glUseProgram(combineProgram.GetGPUProgram());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fboHDR.GetBuffer("color"));
        glUniform1i(combineProgram.GetQualfiterLoc("U_MainTexture"), 0);
        glActiveTexture(GL_TEXTURE1);
        // glBindTexture(GL_TEXTURE_2D, fboHDR.GetBuffer("hdrBuffer"));
        glBindTexture(GL_TEXTURE_2D, blurRes);
        glUniform1i(combineProgram.GetQualfiterLoc("U_HDRTexture"), 1);
        fsq.DrawToQuarter(combineProgram.GetQualfiterLoc("pos"), combineProgram.GetQualfiterLoc("texcoord"), 0);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0); // 重置
        glFinish();
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

    glDisable(GL_BLEND);
    return 0;
}

void GLRenderer::BlurInit(int viewW, int viewH)
{
    mSimpGaussProgramH = new GPUProgram();
    mSimpGaussProgramV = new GPUProgram();

    mBlurFbo = new FBO[2]();
    
    mBlurFbo[0].AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    mBlurFbo[0].AttachDepthBuffer("depth", viewW, viewH);
    mBlurFbo[0].Finish();

	mBlurFbo[1].AttachColorBuffer("color", GL_COLOR_ATTACHMENT0, GL_RGBA, viewW, viewH);
    mBlurFbo[1].AttachDepthBuffer("depth", viewW, viewH);
    mBlurFbo[1].Finish();

    mSimpGaussProgramH->AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    // mSimpGaussProgramH->AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/gaussianHorizontal.fs"));
    mSimpGaussProgramH->AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/gaussian.fs"));
    mSimpGaussProgramH->Link();
    mSimpGaussProgramH->DetectAttributes({"pos", "texcoord"});
    mSimpGaussProgramH->DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;

    mSimpGaussProgramV->AttachShader(GL_VERTEX_SHADER, S_PATH("shader/fullscreenQuad.vs"));
    // mSimpGaussProgramV->AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/gaussianVertical.fs"));
    mSimpGaussProgramV->AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/gaussian.fs"));
    mSimpGaussProgramV->Link();
    mSimpGaussProgramV->DetectAttributes({"pos", "texcoord"});
    mSimpGaussProgramV->DetectUniforms({"U_MainTexture"});
    GL_CHECK_ERROR;
}

GLuint GLRenderer::Blur(FullScreenQuad& fsq, GLuint texture, int blurCount)
{
    GLuint needBuffer = texture;

    for (int i = 0; i < blurCount; ++i)
    {
        // 进行两次高斯模糊
        mBlurFbo[0].Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(mSimpGaussProgramH->GetGPUProgram());
        glBindTexture(GL_TEXTURE_2D, needBuffer);
        glUniform1i(mSimpGaussProgramH->GetQualfiterLoc("U_MainTexture"), 0);
        fsq.Draw(mSimpGaussProgramH->GetQualfiterLoc("pos"), mSimpGaussProgramH->GetQualfiterLoc("texcoord"));
        mBlurFbo[0].UnBind();

        mBlurFbo[1].Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(mSimpGaussProgramV->GetGPUProgram());
        glBindTexture(GL_TEXTURE_2D, mBlurFbo[0].GetBuffer("color"));
        glUniform1i(mSimpGaussProgramV->GetQualfiterLoc("U_MainTexture"), 0);
        fsq.Draw(mSimpGaussProgramV->GetQualfiterLoc("pos"), mSimpGaussProgramV->GetQualfiterLoc("texcoord"));
        mBlurFbo[1].UnBind();

        needBuffer = mBlurFbo[1].GetBuffer("color");
    }

    return needBuffer;
}

void GLRenderer::GetRendererObject(GLuint& vao, GLuint& vbo, GLuint& ebo)
{
	vao = this->VAO;
	vbo = this->VBO;
	ebo = this->EBO;
}


#if 1
GLuint GLRenderer::CreateTextureFromFile(const char* filePath)
{
    GLuint texture = SOIL_load_OGL_texture(filePath, 0, 0, SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_INVERT_Y);
    return texture;
}
#else
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
#endif
