#include "win32_common.h"
#include "GL/glew.h"
#include "GL/wglew.h"


Win32Utils::Win32Utils(/* args */)
{
}

Win32Utils::~Win32Utils()
{
}


LRESULT CALLBACK GLWindowProc(HWND hwnd, UINT msg, WPARAM wparm, LPARAM lparm)
{
	switch (msg)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	default:
		break;
	}
	return DefWindowProc(hwnd, msg, wparm, lparm);
}

HWND Win32Utils::CreateWin32Window(const int width, const int height)
{
    HINSTANCE hinstance = GetModuleHandle(NULL);

    //注册窗口
	WNDCLASSEX wndClass;
	wndClass.cbClsExtra = 0;
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.cbWndExtra = 0;
	wndClass.hbrBackground = NULL;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hIcon = NULL; //LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
	wndClass.hIconSm = NULL;
	wndClass.hInstance = hinstance;
	wndClass.lpfnWndProc = GLWindowProc;
	wndClass.lpszClassName = L"OpenGL";
	wndClass.lpszMenuName = NULL;
	wndClass.style = CS_VREDRAW | CS_HREDRAW;
	ATOM atom = RegisterClassEx(&wndClass);

    // Place the window in the middle of the screen.
	int posX = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
	int posY = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

	this->hwnd = CreateWindowEx(NULL, L"OpenGL", L"RenderWindow", 
                                WS_OVERLAPPEDWINDOW, posX, posY, 
                                width, height, NULL, NULL, hinstance, NULL);

    return this->hwnd;
}

HDC Win32Utils::bindWindowWithOpenGL()
{
	//----OpenGL 绑定windows窗口----------
	this->dc = GetDC(this->hwnd);
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
	return this->dc;
}

HDC Win32Utils::bindWindowWithOpenGL_3_0()
{
	HINSTANCE hinstance = GetModuleHandle(NULL);
	HWND hWndFake = CreateWindow(L"Simple_openGL_class", L"FAKE", WS_OVERLAPPEDWINDOW | WS_MAXIMIZE | WS_CLIPCHILDREN,
		0, 0, CW_USEDEFAULT, CW_USEDEFAULT, NULL,
		NULL, hinstance, NULL);
	this->dc = GetDC(hWndFake);

	// First, choose false pixel format
	//PIXELFORMATDESCRIPTOR pfd;
	//memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
	//pfd.nVersion = 1;
	//pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_TYPE_RGBA | PFD_DOUBLEBUFFER;
	//pfd.iLayerType = PFD_MAIN_PLANE;
	//pfd.iPixelType = PFD_TYPE_RGBA;
	//pfd.cColorBits = 32;
	//pfd.cDepthBits = 24;
	//pfd.cStencilBits = 8;

	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 32;
	pfd.iLayerType = PFD_MAIN_PLANE;

	int pixelFormatID = ChoosePixelFormat(dc, &pfd);
	if (FALSE == SetPixelFormat(dc, pixelFormatID, &pfd))
	{
		MessageBox(hwnd, L"SetPixelFormat error", L"Fatal Error", MB_ICONERROR);
		return NULL;
	}

	HGLRC hRCFake = wglCreateContext(dc);
	wglMakeCurrent(dc, hRCFake);

	if (glewInit() != GLEW_OK)
	{
		MessageBox(hwnd, L"Couldn't initialize GLEW!", L"Fatal Error", MB_ICONERROR);
		return NULL;
	}

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(hRCFake);
	DestroyWindow(hWndFake);

	return this->dc;
}

HDC Win32Utils::InitOpenGL_3_0(int iMajorVersion, int iMinorVersion)
{
	if (bindWindowWithOpenGL_3_0() == NULL)return NULL;

	this->dc = GetDC(hwnd);

	bool bError = false;
	PIXELFORMATDESCRIPTOR pfd;
	if (iMajorVersion <= 2)
	{
		memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 32;
		pfd.cDepthBits = 32;
		pfd.iLayerType = PFD_MAIN_PLANE;

		int iPixelFormat = ChoosePixelFormat(dc, &pfd);
		if (iPixelFormat == 0)return false;

		if (!SetPixelFormat(dc, iPixelFormat, &pfd))return false;

		// Create the old style context (OpenGL 2.1 and before)
		hRC = wglCreateContext(dc);
		if (hRC)wglMakeCurrent(dc, hRC);
		else bError = true;
	}
	else if (WGLEW_ARB_create_context && WGLEW_ARB_pixel_format)
	{
		const int iPixelFormatAttribList[] =
		{
			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
			WGL_COLOR_BITS_ARB, 32,
			WGL_DEPTH_BITS_ARB, 24,
			WGL_STENCIL_BITS_ARB, 8,
			0 // End of attributes list
		};
		int iContextAttribs[] =
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, iMajorVersion,
			WGL_CONTEXT_MINOR_VERSION_ARB, iMinorVersion,
			WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
			0 // End of attributes list
		};

		int iPixelFormat, iNumFormats;
		wglChoosePixelFormatARB(dc, iPixelFormatAttribList, NULL, 1, &iPixelFormat, (UINT*)&iNumFormats);

		// PFD seems to be only redundant parameter now
		if (!SetPixelFormat(dc, iPixelFormat, &pfd))return false;

		hRC = wglCreateContextAttribsARB(dc, 0, iContextAttribs);
		// If everything went OK
		if (hRC) wglMakeCurrent(dc, hRC);
		else bError = true;

	}
	else 
		bError = true;

	if (bError)
	{
		// Generate error messages
		//char sErrorMessage[255], sErrorTitle[255];
		//sprintf(sErrorMessage, "OpenGL %d.%d is not supported! Please download latest GPU drivers!", iMajorVersion, iMinorVersion);
		//sprintf(sErrorTitle, "OpenGL %d.%d Not Supported", iMajorVersion, iMinorVersion);
		//MessageBox(hwnd, sErrorMessage, sErrorTitle, MB_ICONINFORMATION);
		MessageBox(hwnd, L"OpenGL Not Supported", L"Fatal Error", MB_ICONERROR);
		return NULL;
	}

	return this->dc;
}
