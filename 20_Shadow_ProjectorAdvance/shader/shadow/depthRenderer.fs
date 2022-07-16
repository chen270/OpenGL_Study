#version 330 core

in vec2 V_Texcoord;
uniform sampler2D U_MainTexture;//纹理填充

void main()
{
    float depthValue = texture(U_MainTexture, V_Texcoord).r;
    gl_FragColor = vec4(vec3(pow(depthValue, 2.0)), 1.0);
}