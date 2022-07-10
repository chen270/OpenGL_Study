#version 330 core

in vec2 V_Texcoord;
uniform sampler2D U_MainTexture;//纹理填充

void main()
{
    vec4 maxValue = vec4(0.0);
    int coreSize = 3;
    int halfCoreSize = coreSize / 2;
    float texelOffset = 1.0 / 100.0;
    for(int y = 0; y < coreSize; ++y)
    {
        for(int x = 0; x < coreSize; ++x)
        {
            vec4 color = texture2D(U_MainTexture,V_Texcoord + vec2((-halfCoreSize+x)*texelOffset,(-halfCoreSize+y)*texelOffset));
            maxValue = max(color, maxValue);
        }
    }

    gl_FragColor=maxValue;
}