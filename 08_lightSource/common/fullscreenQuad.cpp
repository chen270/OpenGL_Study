#include "fullscreenQuad.h"

FullScreenQuad::FullScreenQuad(/* args */)
{
}

FullScreenQuad::~FullScreenQuad()
{
}

void FullScreenQuad::Init()
{
    float pos[] = {
        -0.5f, -0.5f, -1.0f,
         0.5f, -0.5f, -1.0f,
         0.5f,  0.5f, -1.0f,
        -0.5f,  0.5f, -1.0f,
    };

    glGenBuffers(1, &mVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*12, pos, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void FullScreenQuad::Draw(GLuint posLoc)
{
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(float)*3, 0);
    glDrawArrays(GL_QUADS, 0, 4);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
