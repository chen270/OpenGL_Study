#version 330 core

in vec3 V_Normal;
in vec4 V_WorldPos;
in vec4 V_EyeSpacePos; // cal Fog

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

uniform float U_FogStart;
uniform float U_FogEnd;
uniform vec4 U_FogColor;


float CalculateLinearFog(float dis)
{
    float factor = (dis - U_FogStart) / (U_FogEnd - U_FogStart); // 求出百分比
    factor = 1.0 - clamp(factor, 0.0, 1.0); // 规定数值在 0.0 ~ 1.0 之间
    return factor; // 雾的 alpha 值
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
        float linearFactor = 0.01;
        float expFactor = 0.01;
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
        diffuseIntensity = max(0.0, dot(L,n)) * 4.0;
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

    // gl_FragColor = ambientColor + diffuseColor*attenuation + specularColor*attenuation;

    // 这里距离是算 从眼睛到点的z维度的距离，不考虑锥形的那种距离
    float fogAlpha = CalculateLinearFog(abs(V_EyeSpacePos.z/V_EyeSpacePos.w));
    gl_FragColor = mix(U_FogColor, ambientColor + diffuseColor*attenuation + specularColor*attenuation, fogAlpha);
}