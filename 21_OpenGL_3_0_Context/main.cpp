#include <windows.h>
#include <iostream>
#include "GL/glew.h"
#include "utils/win32_common.h"
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

	HDC hdc;
	HWND hwnd;
	//utils.InitWindowAndGLContext(hdc, hwnd, 800, 600);
	utils.InitWindowAndGL_3_0_Context(hdc, hwnd, 800, 600);
	glRender.GLInit();

	glClearColor(41.0f/255.0f, 71.0f/255.0f, 121.0f/255.0f, 1.0f);

	GLuint VBO, VAO, EBO;
	GLuint program;
	glRender.initTriangle();
	glRender.GetRendererObject(VAO, VBO, EBO);
	program = glRender.CreateGPUProgram(S_PATH("triangle.vs"), S_PATH("triangle.frag"));
	// ----OpenGL end   ----------

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

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
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(program);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);

		SwapBuffers(hdc);
		// ----OpenGL end  -----
	}

    return 0;
}