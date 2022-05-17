# version 330 core
attribute vec3 pos;
attribute vec2 texcoord;
attribute vec3 normal;


uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

uniform mat4 NM;

in vec4 V_Color;

out vec3 V_Normal;
out vec4 V_WorldPos;
out vec2 V_Texcoord;

void main()
{
    V_Normal = mat3(NM) * normal;//局部坐标系转成世界坐标系
    
    V_WorldPos = M * vec4(pos, 1.0);//当前点的世界坐标

    V_Texcoord = texcoord;

    gl_Position=P*V*M*vec4(pos,1.0);
}