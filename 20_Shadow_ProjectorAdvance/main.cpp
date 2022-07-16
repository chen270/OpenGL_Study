#include <windows.h>
#include <iostream>
#include "glew/glew.h"
#include <gl/GL.h>
#include "common/win32_common.h"
#include "common/fixRenderer.h"
#include "common/glRenderer.h"
#include "common/misc.h"

int main()
{
    Win32Utils utils;
    GLRenderer glRender;

    int width = 1280;
    int height = 720;


    HWND hwnd = utils.CreateWin32Window(width, height);
    HDC dc = utils.bindWindowWithOpenGL(width, height);
    glRender.GLInit();

#if 0
    glClearColor(41.0f / 255.0f, 71.0f / 255.0f, 121.0f / 255.0f, 1.0f);

    // FixRenderer::Rendererinit();
    glRender.InitModel();
    glRender.InitFullScreenQuad();
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
        glRender.UpdateModel(angle);
        SwapBuffers(dc);
        // ----OpenGL end  -----
    }

#else
    // glRender.LightDiffusePixelEnv(hwnd, dc);
    // glRender.LightSpecularPixelEnv(hwnd, dc, width, height);
    // glRender.Erosion_Dilation_MultiGuassian(hwnd, dc, width, height);
    glRender.Projector_Advance(hwnd, dc, width, height);
    // glRender.LightDiffuseVertexEnv(hwnd, dc);
#endif
    return 0;
}