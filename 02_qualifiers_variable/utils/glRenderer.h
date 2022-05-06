#include <windows.h>
#include <gl/GL.h>

class GLRenderer
{
public:
    GLRenderer(/* args */);
    ~GLRenderer();

    int    GLInit();
    GLuint CreateGPUProgram(const char * vsShaderPath, const char * fsShaderPath);
    int    initTriangle();
    void   GetRendererObject(GLuint& vao, GLuint& vbo, GLuint& ebo);
    void   SetShaderQualifiers();
    static void CheckGLError(const char *file, int line);

private:
    int LoadFileContent(const char* path, char** buf);
    int CheckCompile(GLuint vsShader);

private:
    char infoLog[512];
    GLuint VBO, VAO, EBO;
    GLuint program;


    char* vsCode;
	char* fsCode;
};

