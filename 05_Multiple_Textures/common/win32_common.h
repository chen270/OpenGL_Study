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

private:
    HWND hwnd;
    HDC  dc;
};

#endif // __WIN32_COMMON_H__