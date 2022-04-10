#include "win32_common.h"

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