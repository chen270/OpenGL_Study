#include <windows.h>
#include <gl/GL.h>
#include "utils/win32_common.h"
#include "utils/fixRenderer.h"

int main()
{
    Win32Utils utils;
    HWND hwnd = utils.CreateWin32Window(800, 600);
	HDC dc = utils.bindWindowWithOpenGL();

	glClearColor(41.0f/255.0f, 71.0f/255.0f, 121.0f/255.0f, 1.0f);

    FixRenderer::Rendererinit();
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

		FixRenderer::Draw();

		//glClear(GL_COLOR_BUFFER_BIT);
		SwapBuffers(dc);
		//----OpenGL end  -----
	}

    return 0;
}