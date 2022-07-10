#include "fullscreenQuad.h"
#include <vector>
#include <algorithm>

#include "misc.h"

FullScreenQuad::FullScreenQuad(/* args */)
{
}

FullScreenQuad::~FullScreenQuad()
{
}

void FullScreenQuad::Init()
{
    // float vertices[] = {
    //     -0.5f, -0.5f, -1.0f,
    //      0.5f, -0.5f, -1.0f,
    //      0.5f,  0.5f, -1.0f,
    //     -0.5f,  0.5f, -1.0f,
    // };

    glGenBuffers(1, &mVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*20, nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void FullScreenQuad::Draw(GLuint posLoc, GLuint texcoordLoc)
{
    // 全屏画
    float vertices[] = {
        -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, -1.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -1.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, -1.0f, 0.0f, 1.0f,
    };

    glGenBuffers(1, &mVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*20, vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(float)*5, 0);
    glEnableVertexAttribArray(texcoordLoc);
    glVertexAttribPointer(texcoordLoc, 2, GL_FLOAT, GL_FALSE, sizeof(float)*5, reinterpret_cast<void*>(sizeof(float)*3));
    glDrawArrays(GL_QUADS, 0, 4);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

enum QuarterPos
{
    LeftTop = 0,
    LeftBottom = 1,
    RightTop = 2,
    RightBottom = 3,
};

void FullScreenQuad::DrawToQuarter(GLuint posLoc, GLuint texcoordLoc, int in)
{
    std::vector<float>verticesPre; // vertPos + texcoord
    enum QuarterPos quarter = (QuarterPos) in;
    switch (quarter)
    {
    case LeftTop:
        verticesPre = {
            -0.5f, 0.0f, -1.0f, 0.0f, 0.0f,
             0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
             0.0f, 0.5f, -1.0f, 1.0f, 1.0f,
            -0.5f, 0.5f, -1.0f, 0.0f, 1.0f,
        };
        break;
    case LeftBottom:
        verticesPre = {
            -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
             0.0f, -0.5f, -1.0f, 1.0f, 0.0f,
             0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
            -0.5f,  0.0f, -1.0f, 0.0f, 1.0f,
        };
        break;
    case RightTop:
        verticesPre = {
             0.0f,  0.0f, -1.0f, 0.0f, 0.0f,
             0.5f,  0.0f, -1.0f, 1.0f, 0.0f,
             0.5f,  0.5f, -1.0f, 1.0f, 1.0f,
             0.0f,  0.5f, -1.0f, 0.0f, 1.0f,
        };
        break;
    case RightBottom:
        verticesPre = {
            0.0f, -0.5f, -1.0f, 0.0f, 0.0f,
            0.5f, -0.5f, -1.0f, 1.0f, 0.0f,
            0.5f,  0.0f, -1.0f, 1.0f, 1.0f,
            0.0f,  0.0f, -1.0f, 0.0f, 1.0f,
        };
        break;
    default:
        // 全屏画
        verticesPre = {
            -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
             0.5f, -0.5f, -1.0f, 1.0f, 0.0f,
             0.5f,  0.5f, -1.0f, 1.0f, 1.0f,
            -0.5f,  0.5f, -1.0f, 0.0f, 1.0f,
        };
        break;
    }
    float pos[20];
    std::copy(verticesPre.begin(), verticesPre.end(), pos);

    glGenBuffers(1, &mVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*20, pos, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(float)*5, 0);
    glEnableVertexAttribArray(texcoordLoc);
    glVertexAttribPointer(texcoordLoc, 2, GL_FLOAT, GL_FALSE, sizeof(float)*5, reinterpret_cast<void*>(sizeof(float)*3));
    glDrawArrays(GL_QUADS, 0, 4);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_ERROR;
}