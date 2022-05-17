#include <windows.h>
#include <iostream>
#include "glew/glew.h"
#include <gl/GL.h>
#include "utils/win32_common.h"
#include "utils/fixRenderer.h"
#include "utils/glRenderer.h"
#include "utils/misc.h"

int main()
{
	Win32Utils utils;
	GLRenderer glRender;

	HWND hwnd = utils.CreateWin32Window(800, 600);
	HDC dc = utils.bindWindowWithOpenGL();
	glRender.GLInit();

	glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);

	// FixRenderer::Rendererinit();
	//GLuint VBO, VAO, EBO;
	GLuint program;
	program = glRender.CreateGPUProgram(S_PATH("shader/texture.vs"), S_PATH("shader/texture.fs"));
	ShaderParameters sp;
	glRender.InitModel(&sp);
	// glRender.SetTriangle_ShaderQualifiers();
	// ----OpenGL end   ----------

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	// model msg
	float angle = 0;

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
		glRender.UpdateModel(sp, angle);
		SwapBuffers(dc);
		// ----OpenGL end  -----
	}

	return 0;
}