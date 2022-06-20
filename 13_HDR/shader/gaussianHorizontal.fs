#version 330 core

in vec2 V_Texcoord;
uniform sampler2D U_MainTexture;//纹理填充

void main()
{
    vec4 color = vec4(0.0);
    float weight[5] = float[](0.22, 0.19, 0.121, 0.08, 0.01);
    vec2  texSize = 1.0 / textureSize(U_MainTexture, 0);
    float texelOffset = texSize.x;
    // float texelOffset = 1.0 / 400.0;
    for(int i = 0; i < 5; ++i)
    {
        color += texture2D(U_MainTexture, vec2(V_Texcoord.x + texelOffset*i, V_Texcoord.y)) * weight[i];
        color += texture2D(U_MainTexture, vec2(V_Texcoord.x - texelOffset*i, V_Texcoord.y)) * weight[i];
    }

    gl_FragColor= color;
}