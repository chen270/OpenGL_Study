#include "fixRenderer.h"
#include <windows.h>
#include <gl/GL.h>
#include <gl/GLU.h> //固定管线头文件

#pragma comment(lib,"GlU32.Lib")

FixRenderer::FixRenderer(/* args */)
{
}

FixRenderer::~FixRenderer()
{
}

void FixRenderer::Rendererinit()
{
    glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, 800.0f / 600.0f, 0.1f, 1000.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void FixRenderer::Draw()
{
    glClear(GL_COLOR_BUFFER_BIT);

    glBegin(GL_TRIANGLES);
    glVertex3f(0, 0, -100.0f);
    glVertex3f(10, 0, -100.0f);
    glVertex3f(0, 10, -100.0f);

    glEnd();
}