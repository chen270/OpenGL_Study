#ifndef __MISC_H__
#define __MISC_H__

#ifndef DIR_PATH
#error shader path not define!
#else
#define S_PATH(str) DIR_PATH##str
#endif

struct VertexData
{
    float pos[3];
    float texcoord[2];
    float normal[3];
};

class misc
{
public:
    misc(/* args */);
    ~misc();

    static int LoadFileContent(const char* path, char** buf);
    static VertexData * LoadObjModel(const char* filePath, unsigned int ** index, int & vertexCount, int & indexCount);
    static unsigned char *misc::DecodeBMPData(unsigned char *imageData, int &width, int &height);
    static unsigned char *DecodeDXT1Data(unsigned char *imageData, int &width, int &height, int &pixelDataSize);
};

#endif // __MISC_H__