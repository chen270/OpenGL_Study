#version 330 core

in vec2 V_Texcoord;
uniform sampler2D U_MainTexture;//纹理填充

void main()
{
    vec4 color = vec4(0.0);
    int coreSize = 3;
    int halfCoreSize = coreSize / 2;
    float texelOffset = 1.0 / 150.0;
    float knerel[9];
    knerel[6] = 0; knerel[7] = -1; knerel[8] = 0;
    knerel[3] = -1; knerel[4] = 4; knerel[5] = -1;
    knerel[0] = 0; knerel[1] = -1; knerel[2] = 0;
    int index = 0;
    for(int y = 0; y < coreSize; ++y)
    {
        for(int x = 0; x < coreSize; ++x)
        {
            vec4 currentColor = texture2D(U_MainTexture,V_Texcoord + vec2((-1+x)*texelOffset,(-1+y)*texelOffset));
            color += currentColor*knerel[index++];
        }
    }

    float factor = 2.0;
    gl_FragColor= factor*color + texture2D(U_MainTexture,V_Texcoord);
}