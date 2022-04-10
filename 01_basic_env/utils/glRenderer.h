#include <windows.h>
#include <gl/GL.h>

class GlRenderer
{
public:
    GlRenderer(/* args */);
    ~GlRenderer();
    GLuint CreateGPUProgram(const char * vsShaderPath, const char * fsShaderPath);

private:
    char* LoadFileContent(const char* path);
};

