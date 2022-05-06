#include <windows.h>


class Win32Utils
{
public:
    Win32Utils(/* args */);
    ~Win32Utils();

    HWND CreateWin32Window(const int width, const int height);
    HDC bindWindowWithOpenGL();

public:
    HWND hwnd;
    HDC  dc;
};

