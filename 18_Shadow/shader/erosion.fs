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

    vec4 minValue = vec4(1.0);
    int coreSize = 5;
    int halfCoreSize = coreSize / 2;
    float texelOffset = 1.0 / 100.0;
    for(int y = 0; y < coreSize; ++y)
    {
        for(int x = 0; x < coreSize; ++x)
        {
            vec4 currentAlpha = texture2D(U_MainTexture,V_Texcoord + vec2((-halfCoreSize+x)*texelOffset,(-halfCoreSize+y)*texelOffset));
            minValue = min(currentAlpha, minValue);
        }
    }

    gl_FragColor=minValue;
}