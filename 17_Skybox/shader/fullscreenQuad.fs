#version 330 core

in vec2 V_Texcoord;
uniform sampler2D U_MainTexture;//纹理填充

void main()
{
    gl_FragColor=texture2D(U_MainTexture,V_Texcoord);
}