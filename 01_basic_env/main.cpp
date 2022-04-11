#include <windows.h>
#include <iostream>
#include "glew/glew.h"
#include <gl/GL.h>
#include "utils/win32_common.h"
#include "utils/fixRenderer.h"
#include "utils/glRenderer.h"

#ifndef SHADER_PATH
#error shader path not define!
#else
#define S_PATH(str) SHADER_PATH##str
#endif

int main()
{
    Win32Utils utils;
	GLRenderer glRender;
	
	HWND hwnd = utils.CreateWin32Window(800, 600);
	HDC dc = utils.bindWindowWithOpenGL();
	glRender.GLInit();

	glClearColor(41.0f/255.0f, 71.0f/255.0f, 121.0f/255.0f, 1.0f);

	//FixRenderer::Rendererinit();
	GLuint VBO, VAO, EBO;
	GLuint program;
	glRender.initTriangle();
	glRender.GetRendererObject(VAO, VBO, EBO);
	program = glRender.CreateGPUProgram(S_PATH("triangle.vs"), S_PATH("triangle.frag"));
	//----OpenGL end   ----------

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	MSG msg;
	//防止程序退出
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

		//----OpenGL start-----
		glClear(GL_COLOR_BUFFER_BIT);

		//FixRenderer::Draw();
		glUseProgram(program);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);

		SwapBuffers(dc);
		//----OpenGL end  -----
	}

    return 0;
}