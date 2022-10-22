#include "win32_common.h"
#include "glew/glew.h"
#include "glew/wglew.h"


Win32Utils::Win32Utils(/* args */)
{
}

Win32Utils::~Win32Utils()
{
}

static LRESULT CALLBACK GLWindowProc(HWND hwnd, UINT msg, WPARAM wparm, LPARAM lparm)
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

static HWND CreateWin32Window(const HINSTANCE _hinstance, const int width, const int height)
{
    //注册窗口
    WNDCLASSEX wndClass;
    wndClass.cbClsExtra = 0;
    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.cbWndExtra = 0;
    wndClass.hbrBackground = NULL;
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hIcon = NULL; // LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
    wndClass.hIconSm = NULL;
    wndClass.hInstance = _hinstance;
    wndClass.lpfnWndProc = GLWindowProc;
    wndClass.lpszClassName = L"OpenGL";
    wndClass.lpszMenuName = NULL;
    wndClass.style = CS_VREDRAW | CS_HREDRAW;
    ATOM atom = RegisterClassEx(&wndClass);

    // Place the window in the middle of the screen.
    int posX = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    int posY = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

    auto hwnd = CreateWindowEx(NULL, L"OpenGL", L"RenderWindow",
                               WS_OVERLAPPEDWINDOW, posX, posY,
                               width, height, NULL, NULL, _hinstance, NULL);

    return hwnd;
}

static bool InitGLEW(const HINSTANCE hInstance, const HWND _hwnd)
{
    HWND hWndFake = CreateWin32Window(hInstance, 800, 600);
    auto _hDC = GetDC(hWndFake);

    // First, choose false pixel format
    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 32;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int iPixelFormat = ChoosePixelFormat(_hDC, &pfd);
    if (iPixelFormat == 0)
        return false;

    if (!SetPixelFormat(_hDC, iPixelFormat, &pfd))
        return false;

    // Create the false, old style context (OpenGL 2.1 and before)
    HGLRC hRCFake = wglCreateContext(_hDC);
    wglMakeCurrent(_hDC, hRCFake);

    bool bResult = true;

    if (glewInit() != GLEW_OK)
    {
        MessageBox(_hwnd, L"Couldn't initialize GLEW!", L"Fatal Error", MB_ICONERROR);
        bResult = false;
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hRCFake);
    DestroyWindow(hWndFake);

    return bResult;
}

int Win32Utils::InitWindowAndGLContext(HDC &_hdc, HWND &_hwnd, const int width, const int height)
{
    // Window Init
    auto hinstance = GetModuleHandle(NULL);
    _hwnd = CreateWin32Window(hinstance, width, height);
    _hdc = GetDC(_hwnd);
    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));

    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_TYPE_RGBA | PFD_DOUBLEBUFFER;
    pfd.iLayerType = PFD_MAIN_PLANE;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;

    int pixelFormatID = ChoosePixelFormat(_hdc, &pfd);
    SetPixelFormat(_hdc, pixelFormatID, &pfd);

    HGLRC rc = wglCreateContext(_hdc);
    wglMakeCurrent(_hdc, rc);

    // glew Init
    if (glewInit() != GLEW_OK)
    {
        MessageBox(_hwnd, L"Failed to initialize GLEW", L"Fatal Error", MB_ICONINFORMATION);
        return -1;
    }

    return 0;
}

int Win32Utils::InitWindowAndGL_3_0_Context(HDC &_hdc, HWND &_hwnd, const int width, const int height,
                                            const int iMajorVersion, const int iMinorVersion) // default 3.3 version
{
    auto hinstance = GetModuleHandle(NULL);
    _hwnd = CreateWin32Window(hinstance, 800, 600);

    if (!InitGLEW(hinstance, _hwnd))
        return -1;

    _hdc = GetDC(_hwnd);

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

        int iPixelFormat = ChoosePixelFormat(_hdc, &pfd);
        if (iPixelFormat == 0)
            return false;

        if (!SetPixelFormat(_hdc, iPixelFormat, &pfd))
            return false;

        // Create the old style context (OpenGL 2.1 and before)
        m_hRC = wglCreateContext(_hdc);
        if (m_hRC)
            wglMakeCurrent(_hdc, m_hRC);
        else
            bError = true;
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
        wglChoosePixelFormatARB(_hdc, iPixelFormatAttribList, NULL, 1, &iPixelFormat, (UINT *)&iNumFormats);

        // PFD seems to be only redundant parameter now
        if (!SetPixelFormat(_hdc, iPixelFormat, &pfd))
            return false;

        m_hRC = wglCreateContextAttribsARB(_hdc, 0, iContextAttribs);
        // If everything went OK
        if (m_hRC)
            wglMakeCurrent(_hdc, m_hRC);
        else
            bError = true;
    }
    else
        bError = true;

    if (bError)
    {
        // Generate error messages
        wchar_t sErrorMessage[255], sErrorTitle[255];
        wsprintf(sErrorMessage, L"OpenGL %d.%d is not supported! Please download latest GPU drivers!", iMajorVersion, iMinorVersion);
        wsprintf(sErrorTitle, L"OpenGL %d.%d Not Supported", iMajorVersion, iMinorVersion);
        MessageBox(_hwnd, sErrorMessage, sErrorTitle, MB_ICONINFORMATION);
        return NULL;
    }

    return 0;
}
