#version 330 core

in vec3 V_Normal;
in vec4 V_WorldPos;
in vec2 V_Texcoord;
in vec4 V_ProjectCoord;
in vec4 V_PorjectSpaceFragPos;

uniform sampler2D U_MainTexture; // 物体本身的纹理
uniform sampler2D U_ProjectiveTexture; // 用来投影到物体的纹理
uniform sampler2D U_ShadowMap;


uniform vec4 U_LightPos;
float CalculateShadow()
{
    vec3 fragPos = V_PorjectSpaceFragPos.xyz / V_PorjectSpaceFragPos.w; // 结果 -1 ~ 1
    fragPos = fragPos * 0.5 + vec3(0.5); // 取范围到 0 ~ 1
    float depthInShadowmap = texture(U_ShadowMap, fragPos.xy).r;
    float currentDepth = fragPos.z; // 当前摄像机下的深度

    float bias = 0.001;
    // vec3 normal = normalize(V_Normal); // n vector
    // vec3 lightDir = normalize(U_LightPos.xyz - V_WorldPos.xyz);
    // float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    // 普通
    float shadow = currentDepth - bias > depthInShadowmap ? 1.0 : 0.0;
    return shadow;
}

void main()
{
    if (V_ProjectCoord.z > 0.0) // 在投影仪朝向内
    {
        vec4 color1 = textureProj(U_ProjectiveTexture,V_ProjectCoord);
        if (color1.a == 0.0)
		    gl_FragColor=texture2D(U_MainTexture,V_Texcoord);
        else
		    gl_FragColor=vec4(0.0,0.0,0.0, textureProj(U_ProjectiveTexture,V_ProjectCoord).a);
        // gl_FragColor=0.5*texture2D(U_MainTexture,V_Texcoord) + textureProj(U_ProjectiveTexture,V_ProjectCoord) * 0.5 * vec4(1.0-shadow);
        // gl_FragColor=vec4(texture2D(U_MainTexture,V_Texcoord).rgb, (textureProj(U_ProjectiveTexture,V_ProjectCoord) * vec4(1.0-shadow)).a);

        // gl_FragColor=textureProj(U_ProjectiveTexture,V_ProjectCoord);
    }
    else
    {
        gl_FragColor=texture2D(U_MainTexture,V_Texcoord);
    }
}