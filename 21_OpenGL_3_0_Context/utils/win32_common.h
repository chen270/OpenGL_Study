#ifndef __WIN32_COMMON_H__
#define __WIN32_COMMON_H__

#include <windows.h>

class Win32Utils
{
public:
    Win32Utils(/* args */);
    ~Win32Utils();

    HWND CreateWin32Window(const int width, const int height);
    HDC bindWindowWithOpenGL();
    HDC bindWindowWithOpenGL_3_0();
    HDC InitOpenGL_3_0(int iMajorVersion, int iMinorVersion);

public:
    HWND hwnd;
    HDC  dc;
	HGLRC hRC;

};

#endif // __WIN32_COMMON_H__