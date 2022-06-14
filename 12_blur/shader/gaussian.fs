#version 330 core

in vec2 V_Texcoord;
uniform sampler2D U_MainTexture;//纹理填充

void main()
{
    // float minValue = 1.0;
    // int coreSize = 40;
    // int halfCoreSize = coreSize / 2;
    // float texelOffset = 1.0 / 100.0;
    // for(int y = 0; y < coreSize; ++y)
    // {
    //     for(int x = 0; x < coreSize; ++x)
    //     {
    //         float currentAlpha = texture2D(U_MainTexture,V_Texcoord + vec2((-halfCoreSize+x)*texelOffset,(-halfCoreSize+y)*texelOffset)).a;
    //         minValue = min(currentAlpha, minValue);
    //     }
    // }

    // gl_FragColor=vec4(texture2D(U_MainTexture,V_Texcoord).rgb, minValue);

    vec4 color = vec4(0.0);
    int coreSize = 3;
    int halfCoreSize = coreSize / 2;
    float texelOffset = 1.0 / 200.0;
    float knerel[9];
    knerel[6] = 1; knerel[7] = 2; knerel[8] = 1;
    knerel[3] = 2; knerel[4] = 4; knerel[5] = 2;
    knerel[0] = 1; knerel[1] = 2; knerel[2] = 1;
    int index = 0;
    for(int y = 0; y < coreSize; ++y)
    {
        for(int x = 0; x < coreSize; ++x)
        {
            vec4 currentColor = texture2D(U_MainTexture,V_Texcoord + vec2((-1+x)*texelOffset,(-1+y)*texelOffset));
            color += currentColor*knerel[index++];
        }
    }
    color /= 16.0;
    gl_FragColor=color;
}