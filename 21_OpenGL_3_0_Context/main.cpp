#include <windows.h>
#include <iostream>
#include "GL/glew.h"
#include "GL/wglew.h"
#include <gl/GL.h>
#include "utils/win32_common.h"
#include "utils/glRenderer.h"

#ifndef SHADER_PATH
#error shader path not define!
#else
#define S_PATH(str) SHADER_PATH##str
#endif

int main()
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	auto hWnd = CreateWindowEx(NULL, L"OpenGL", L"RenderWindow",
		WS_OVERLAPPEDWINDOW, 0, 0,
		512, 512, NULL, NULL, hInstance, NULL);

	ShowWindow(hWnd, SW_SHOW);

	// Just to send WM_SIZE message again
	//ShowWindow(hWnd, SW_SHOWMAXIMIZED);
	UpdateWindow(hWnd);


	return 0;
}

int main2()
{
    Win32Utils utils;
	GLRenderer glRender;

	HWND hwnd = utils.CreateWin32Window(800, 600);
	//HDC dc = utils.bindWindowWithOpenGL();
	HDC dc = utils.InitOpenGL_3_0(3,3);
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

		SwapBuffers(dc);
		// ----OpenGL end  -----
	}

    return 0;
}