#include "fbo.h"
#include <iostream>

FBO::FBO(/* args */)
{
    glGenFramebuffers(1, &mFBO);
}

FBO::~FBO()
{
    glDeleteFramebuffers(1, &mFBO);
}

int FBO::AttachColorBuffer(const char *bufferName, GLenum attachment, GLenum dataType, int width, int height)
{
    int ret = 0;
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    // 创建color buffer,纹理
    GLuint colorBuffer;
	glGenTextures(1, &colorBuffer);
	glBindTexture(GL_TEXTURE_2D, colorBuffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D, 0, dataType, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);

    // 将它附加到当前绑定的帧缓冲对象
	glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, colorBuffer, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

    // 检查帧缓冲是否完整
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
		// 执行胜利的舞蹈
		std::cout << "glFramebuffer init success" << std::endl;
	}
    else
    {
        std::cout << "glFramebuffer init error" << std::endl;
        ret = -1;
    }

	// 解绑帧缓冲，保证我们不会不小心渲染到错误的帧缓冲上
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

    mDrawBuffers.push(attachment);
    mBuffers.insert(std::pair<std::string, GLuint>(bufferName, colorBuffer));
    return ret;
}

int FBO::AttachDepthBuffer(const char *bufferName, int width, int height)
{
    int ret = 0;

    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);

    GLuint depthBuffer;
	glGenTextures(1, &depthBuffer);
	glBindTexture(GL_TEXTURE_2D, depthBuffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);

    //fbo绑定depth buffer
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthBuffer, 0);

    // 检查帧缓冲是否完整
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
		// 执行胜利的舞蹈
		std::cout << "glFramebuffer init success" << std::endl;
	}
    else
    {
        std::cout << "glFramebuffer init error" << std::endl;
        ret = -1;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    mBuffers.insert(std::pair<std::string, GLuint>(bufferName, depthBuffer));
    return ret;
}

// complete FBO setting
int FBO::Finish()
{
    int nCount = mDrawBuffers.size();
    if(nCount < 1)
        return -1;

    GLenum* buffers = new GLenum[nCount]();
    int index = 0;
    while(!mDrawBuffers.empty())
    {
        buffers[index] = mDrawBuffers.top();
        mDrawBuffers.pop();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    glDrawBuffers(nCount, buffers);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    delete[] buffers;
    return 0;
}

GLuint FBO::GetBuffer(const char *bufferName)
{
    auto it = mBuffers.find(bufferName);
    if(it == mBuffers.end())
        return 0;
    else
        return it->second;
}

GLuint FBO::GetFBO()
{
    return this->mFBO;
}

void FBO::Bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
}

void FBO::UnBind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
