#ifndef __FBO_H__
#define __FBO_H__

#include "glew/glew.h"

#include <unordered_map>
#include <string>
#include <stack>

class FBO
{
public:
    FBO(/* args */);
    ~FBO();

    int AttachColorBuffer(const char* bufferName, GLenum attachment, GLenum dataType, int width, int height);
    int AttachDepthBuffer(const char* bufferName, int width, int height);

    // complete FBO setting
    int Finish();

    GLuint GetBuffer(const char* bufferName);
    GLuint GetFBO();
    void Bind();
    void UnBind();
private:
    std::unordered_map<std::string, GLuint> mBuffers;
    std::stack<GLenum> mDrawBuffers;
    GLuint mFBO;
};






#endif // __FBO_H__
