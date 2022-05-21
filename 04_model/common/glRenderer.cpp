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

int GLRenderer::InitModel(ShaderParameters* sp)
{
    // 0.get Program
    if (nullptr == gpuProgram)
        return 0;
    gpuProgram->AttachShader(GL_VERTEX_SHADER, S_PATH("shader/texture.vs"));
    gpuProgram->AttachShader(GL_FRAGMENT_SHADER, S_PATH("shader/texture.fs"));
    gpuProgram->Link();
    GLuint program = gpuProgram->GetGPUProgram();

    // 1.model
    objModel->InitModel(S_PATH("resource/model/Cube.obj"));

    // 2.传递参数到shader
    gpuProgram->DetectAttributes({"pos", "texcoord", "normal" });
    sp->posLoc = gpuProgram->GetQualfiterLoc("pos");
    sp->texcoordLoc = gpuProgram->GetQualfiterLoc("texcoord");
    sp->normalLoc = gpuProgram->GetQualfiterLoc("normal");

    gpuProgram->DetectUniforms({ "M", "V", "P", "NM", "U_MainTexture" });
    sp->MLoc = gpuProgram->GetQualfiterLoc("M");
    sp->VLoc = gpuProgram->GetQualfiterLoc("V");
    sp->PLoc = gpuProgram->GetQualfiterLoc("P");
    sp->NormalMatrixLoc = gpuProgram->GetQualfiterLoc("NM");
    sp->textureLoc = gpuProgram->GetQualfiterLoc("U_MainTexture");
    GL_CHECK_ERROR;

    // 3.根据图片创建纹理
    sp->imgTex = CreateTextureFromFile(S_PATH("resource/image/niutou.bmp"));

    // 4.初始化 mvp
    sp->modelMsg.mvp = new MVPMatrix();
    sp->modelMsg.mvp->model = glm::translate(0.0f, 0.0f, -4.0f);
    sp->modelMsg.mvp->projection = glm::perspective(45.0f, 800.0f / 600.0f, 0.1f, 1000.0f);
    sp->modelMsg.mvp->normalMatrix = glm::inverseTranspose(sp->modelMsg.mvp->model);

    // 5.opengl 环境设置
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return 0;
}

int GLRenderer::UpdateModel(ShaderParameters& sp, float& angle)
{
    angle += 1.0f;
    if (angle > 360.0f)
        angle = 0;
    glm::mat4 model = glm::translate(0.0f, 0.0f, -4.0f) * glm::rotate(angle, 0.0f, 1.0f, 0.0f);
    glm::mat4 normalMatrix = glm::inverseTranspose(model);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 编译命令
    glUseProgram(gpuProgram->GetGPUProgram());
    glUniformMatrix4fv(sp.MLoc, 1, GL_FALSE, glm::value_ptr(model));      // M model,模型视图移动，
    glUniformMatrix4fv(sp.VLoc, 1, GL_FALSE, identity);                      // V visual 视口
    glUniformMatrix4fv(sp.PLoc, 1, GL_FALSE, glm::value_ptr(sp.modelMsg.mvp->projection)); // 投影
    glUniformMatrix4fv(sp.NormalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    GL_CHECK_ERROR;

    glBindTexture(GL_TEXTURE_2D, sp.imgTex);
    glUniform1i(sp.textureLoc, 0);

    objModel->Bind(sp.posLoc, sp.texcoordLoc, sp.normalLoc);
    objModel->Draw();

    GL_CHECK_ERROR;

    glUseProgram(0); // 重置
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
    misc::LoadFileContent(imagePath, (char**)&imageData);

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