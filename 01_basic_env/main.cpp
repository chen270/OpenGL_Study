#include <windows.h>
#include <gl/GL.h>
#include "utils/win32_common.h"
#include "utils/fixRenderer.h"

int main()
{
    Win32Utils utils;
    HWND hwnd = utils.CreateWin32Window(800, 600);

    //----OpenGL 绑定windows窗口----------
	HDC dc = GetDC(hwnd);
	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));

	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_TYPE_RGBA | PFD_DOUBLEBUFFER;
	pfd.iLayerType = PFD_MAIN_PLANE;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;

	int pixelFormatID = ChoosePixelFormat(dc, &pfd);
	SetPixelFormat(dc, pixelFormatID, &pfd);

	HGLRC rc = wglCreateContext(dc);
	wglMakeCurrent(dc, rc);

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