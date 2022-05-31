#ifndef __FULLSCREENQUAD_H__
#define __FULLSCREENQUAD_H__

#include "glew/glew.h"

// 画铺满窗口的四边形
class FullScreenQuad
{
public:
    FullScreenQuad(/* args */);
    ~FullScreenQuad();
    void Init();
    void Draw(GLuint posLoc);
private:
    /* data */
    GLuint mVBO;
};




#endif // __FULLSCREENQUAD_H__