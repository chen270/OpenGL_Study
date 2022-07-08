#version 330 core

in vec3 V_Normal;
in vec4 V_WorldPos;

uniform vec4 U_DiffuseLightColor;
uniform vec4 U_DiffuseMaterial; // 环境光反射的材质
uniform vec4 U_LightPos;

uniform vec4 U_AmbientLightColor;
uniform vec4 U_AmbientMaterial; // 环境光反射的材质
uniform vec3 U_EyePos;

uniform vec4 U_SpecularLightColor;
uniform vec4 U_SpecularMaterial; // 环境光反射的材质

// 聚光灯
uniform vec4 U_SpotLightDirect;
uniform float U_CutOffAngle;


// Shadow
uniform sampler2D U_ShadowMap;
in vec4 V_LightSpaceFragPos;

float CalculateShadow()
{
    vec3 fragPos = V_LightSpaceFragPos.xyz / V_LightSpaceFragPos.w; // 结果 -1 ~ 1
    fragPos = fragPos * 0.5 + vec3(0.5); // 取范围到 0 ~ 1
    float depthInShadowmap = texture(U_ShadowMap, fragPos.xy).r;
    float currentDepth = fragPos.z; // 当前摄像机下的深度
    float shadow = currentDepth > depthInShadowmap ? 1.0 : 0.0;
    return shadow;
}



void main()
{
    // amibent 
    vec4 ambientColor = U_AmbientLightColor * U_AmbientMaterial; // 环境光

    vec3 L = vec3(0.0);

    // light attribute
    float attenuation = 1.0;


    if (U_LightPos.w == 0.0)
    {
        // direction light, 平行光
        L = U_LightPos.xyz; // L vector
    }
    else
    {
        // 点光源
        // model point -> light pos
        // point light / spot light
        L = U_LightPos.xyz - V_WorldPos.xyz;

        // 点光源衰减因子
        float constFactor = 1.0;
        float linearFactor = 0.2;
        float expFactor = 0.05;
        float dis = length(L);
        attenuation = 1.0 / (constFactor + linearFactor*dis + expFactor*dis*dis);
    }

    // diffuse
    L = normalize(L);
    vec3 n = normalize(V_Normal); // n vector
    float diffuseIntensity = 0.0;
    if (U_LightPos.w != 0.0 && U_CutOffAngle > 0.0)
    {
        // 聚光灯
        float radian = U_CutOffAngle * 3.14 / 180.0;
        float cosPhi = cos(radian);
        vec3 spotDirect = normalize(U_SpotLightDirect.xyz);
        vec3 lightDirect = normalize(-L);
        float cosTheta = dot(spotDirect, lightDirect); // 由于都是单位向量，所以 dot 等于 cos(theta)
        if (cosTheta > cosPhi)
        {
            // cos 是递减函数，所以需要大于
            if (dot(L, n) > 0.0) // 受光部分亮 
            {
                diffuseIntensity = pow(cosTheta, U_SpotLightDirect.w)*2.0;
            }
        }
        else
        {
            diffuseIntensity = 0.0;
        }
    }
    else
    {
        // 点光源
        diffuseIntensity = max(0.0, dot(L,n)) * 2.35;
    }

    vec4 diffuseColor = U_DiffuseLightColor * U_DiffuseMaterial * diffuseIntensity;

    // specular, 采用 blinn-phone 模型
    float specularIntensity = 0.0;
    if(diffuseIntensity == 0.0)
    {
        specularIntensity == 0.0;
    }
    else
    {
        vec3 eye = U_EyePos - V_WorldPos.xyz;
        vec3 viewDir = eye + L;
        viewDir = normalize(viewDir);
        specularIntensity = pow(max(0.0, dot(viewDir, n)), 256.0);
    }
    vec4 specularColor = U_SpecularLightColor * U_SpecularMaterial * specularIntensity;

    vec4 color = (ambientColor + diffuseColor*attenuation + specularColor*attenuation)*1.0;
    // gl_FragColor = (ambientColor + diffuseColor*attenuation + specularColor*attenuation)*2.0;

    gl_FragData[0] = ambientColor + (diffuseColor*attenuation + specularColor*attenuation) * vec4(vec3(1.0 - CalculateShadow()), 1.0); // color
    gl_FragData[1] = vec4(0.0); // hdrBuffer
}