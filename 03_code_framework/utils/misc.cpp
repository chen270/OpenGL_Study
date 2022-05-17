#include "misc.h"
#include <algorithm>
#include <string>
#include <sstream>
#include <vector>

misc::misc(/* args */)
{
}

misc::~misc()
{
}

int misc::LoadFileContent(const char *path, char **buf)
{
    FILE *fp = fopen(path, "rb");
    if (fp != nullptr)
    {
        fseek(fp, 0, SEEK_END);
        int len = ftell(fp);
        if (0 == len)
        {
            printf("file %s is Empty\n", path);
            return -1;
        }
        char *buffer = new char[len + 1]();
        rewind(fp);
        fread(buffer, len + 1, 1, fp);
        fclose(fp);
        // return buffer;
        *buf = buffer;
        return 0;
    }

    printf("open file %s failed\n", path);
    return -1;
}

// 点信息
struct VertexInfo
{
	float v[3];
};

// 面信息
struct VertexDefine
{
	int posIndex;
	int textcoordIndex;
	int normalIndex;

	bool operator==(const VertexDefine & vd) {
		if (vd.posIndex		  == this->posIndex		  &&
			vd.textcoordIndex == this->textcoordIndex &&
			vd.normalIndex    == this->normalIndex)
		{
			return true;
		}
		return false;
	}
};

// Load Obj Model
VertexData *misc::LoadObjModel(const char *filePath, unsigned int **index, int &vertexCount, int &indexCount)
{
    char *fileContent = nullptr;
    LoadFileContent(filePath, &fileContent);
    if (nullptr != fileContent)
    {
        // obj mode decode

        // 三类信息
        std::vector<VertexInfo> positions;
        std::vector<VertexInfo> texcoords;
        std::vector<VertexInfo> normals;
        std::vector<unsigned int> objIndexes; // 对应 opengl indexes
        std::vector<VertexDefine> vertices;   // 最后转换为opengl vertexes

        // 转换成流，用流读的好处在于移植到手机端也不会出现问题
        std::stringstream ssObjFile(fileContent);
        char szOneLine[256]; // 一行行读取

        // 用流来实现字符串转float, 流读取到空格时便会停止
        std::string temp;

        while (!ssObjFile.eof())
        {
            memset(szOneLine, 0, sizeof(szOneLine));
            ssObjFile.getline(szOneLine, 256);
            if (strlen(szOneLine) > 0)
            {
                // printf("echo: %s\n", szOneLine);
                std::stringstream ssOneLine(szOneLine);

                if (szOneLine[0] == 'v')
                {
                    // 读取到空格时便会停止,先读取前缀信息
                    ssOneLine >> temp;

                    if (szOneLine[1] == 't')
                    {
                        // texcoord
                        VertexInfo vi;
                        ssOneLine >> vi.v[0];
                        ssOneLine >> vi.v[1];
                        texcoords.push_back(vi);

                        // printf("read - %s = %f, %f\n", temp.c_str(), vi.v[0], vi.v[1]);
                    }
                    else if (szOneLine[1] == 'n')
                    {
                        // normal
                        VertexInfo vi;
                        ssOneLine >> vi.v[0];
                        ssOneLine >> vi.v[1];
                        ssOneLine >> vi.v[2];
                        normals.push_back(vi);

                        // printf("read - %s = %f, %f, %f\n", temp.c_str(), vi.v[0], vi.v[1], vi.v[2]);
                    }
                    else
                    {
                        // pos
                        // 流读取多次便能读取到所有数据
                        VertexInfo vi;
                        ssOneLine >> vi.v[0];
                        ssOneLine >> vi.v[1];
                        ssOneLine >> vi.v[2];
                        positions.push_back(vi);

                        // printf("read - %s = %f, %f, %f\n", temp.c_str(), vi.v[0], vi.v[1], vi.v[2]);
                    }
                }
                else if (szOneLine[0] == 'f')
                {
                    // face
                    // 读取到空格时便会停止,先读取前缀信息
                    ssOneLine >> temp;
                    temp.clear();
                    for (int i = 0; i < 3; ++i)
                    {
                        ssOneLine >> temp; // 输出点的信息，例: 1/1/1
                        size_t pos = temp.find_first_of('/', 0);
                        std::string posIndexStr = temp.substr(0, pos);

                        size_t pos2 = temp.find_first_of('/', pos + 1);
                        std::string texcoordIndexStr = temp.substr(pos + 1, pos2 - pos - 1);

                        std::string normalIndexStr = temp.substr(pos2 + 1, temp.size() - pos2 - 1);

                        VertexDefine vd;
                        vd.posIndex = atoi(posIndexStr.c_str()) - 1; // 索引从0开始
                        vd.textcoordIndex = atoi(texcoordIndexStr.c_str()) - 1;
                        vd.normalIndex = atoi(normalIndexStr.c_str()) - 1;

                        // check is exist，只存储不同的点
                        int nCurrentIndex = -1;
                        size_t nCurrentVerticeCount = vertices.size();
                        for (size_t j = 0; j < nCurrentVerticeCount; ++j)
                        {
                            if (vd == vertices[j])
                            {
                                nCurrentIndex = j;
                                break;
                            }
                        }

                        if (-1 == nCurrentIndex)
                        {
                            // create new vertice
                            vertices.push_back(vd); // 点集数组
                            nCurrentIndex = nCurrentVerticeCount;
                        }
                        objIndexes.push_back(nCurrentIndex); // 索引数组
                    }

                    // printf("echo: %s\n", szOneLine);
                }
            }
        }

        // printf("face count %u\n", objIndexes.size() / 3);

        // objIndexes -> Indexed Buffer - IBO
        indexCount = static_cast<int>(objIndexes.size());
        *index = new unsigned int[indexCount];
        std::copy(objIndexes.begin(), objIndexes.end(), *index); // index赋值

        // vertices -> Vertexes Buffer - VBO
        vertexCount = static_cast<int>(vertices.size());
        VertexData *vData = new VertexData[vertexCount];
        for (int k = 0; k < vertexCount; ++k)
        {
            memcpy(vData[k].pos, positions[vertices[k].posIndex].v, sizeof(float) * 3);
            memcpy(vData[k].texcoord, texcoords[vertices[k].textcoordIndex].v, sizeof(float) * 2);
            memcpy(vData[k].normal, normals[vertices[k].normalIndex].v, sizeof(float) * 3);
        }

        delete[] fileContent;
        return vData;
    }

    return nullptr;
}

unsigned char *misc::DecodeBMPData(unsigned char *imageData, int &width, int &height)
{
    // decode bmp
    int pixelDataOffset = *(reinterpret_cast<int *>(imageData + 10));
    width = *(reinterpret_cast<int *>(imageData + 18));
    height = *(reinterpret_cast<int *>(imageData + 22));
    unsigned char *pixelData = imageData + pixelDataOffset;
    for (int i = 0; i < width * height * 3; i += 3)
    {
        // bgr -> rgb
        unsigned char tmp = pixelData[i];
        pixelData[i] = pixelData[i + 2];
        pixelData[i + 2] = tmp;
    }

    return pixelData;
}

const unsigned long FORMAT_DXT1 = 0x31545844; // DXT1的码 = 1 -> T -> X -> D 的ascii码
unsigned char *misc::DecodeDXT1Data(unsigned char *imageData, int &width, int &height, int &pixelDataSize)
{
    //硬解码
    height = *(unsigned long *)(imageData + sizeof(unsigned long) * 3);
    width = *(unsigned long *)(imageData + sizeof(unsigned long) * 4);

    pixelDataSize = *(unsigned long *)(imageData + sizeof(unsigned long) * 5);
    unsigned long compressFormat = *(unsigned long *)(imageData + sizeof(unsigned long) * 21);

    switch (compressFormat)
    {
    case FORMAT_DXT1:
        printf("dad\n");
        break;
    default:
        break;
    }

    unsigned char *pixelData = new unsigned char[pixelDataSize];
    memcpy(pixelData, imageData + sizeof(unsigned long) * 10, pixelDataSize);

    return pixelData;
}
