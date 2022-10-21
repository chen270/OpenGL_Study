#ifndef __WIN32_COMMON_H__
#define __WIN32_COMMON_H__

#include <windows.h>

class Win32Utils
{
public:
    Win32Utils(/* args */);
    ~Win32Utils();

    int InitWindowAndGLContext(HDC &_hdc, HWND &_hwnd, const int width, const int height);
    int InitWindowAndGL_3_0_Context(HDC &_hdc, HWND &_hwnd, const int width, const int height,
                                    int iMajorVersion = 3, int iMinorVersion = 3); // default 3.3 version
private:
    HGLRC m_hRC;
};

#endif // __WIN32_COMMON_H__