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

void main()
{
    // amibent 
    vec4 ambientColor = U_AmbientLightColor * U_AmbientMaterial; // 环境光

    vec3 L = vec3(0.0);

    // light attribute
    float attenuation = 1.0;
    float constFactor = 1.0;
    float linearFactor = 0.01;
    float expFactor = 0.01;

    if (U_LightPos.w == 0.0)
    {
        // direction light, 平行光
        L = U_LightPos.xyz; // L vector
    }
    else
    {
        // model point -> light pos
        // point light / spot light
        L = U_LightPos.xyz - V_WorldPos.xyz;

        // 点光源衰减因子
        float dis = length(L);
        attenuation = 1.0 / (constFactor + linearFactor*dis + expFactor*dis*dis);
    }

    // diffuse
    L = normalize(L);
    vec3 n = normalize(V_Normal); // n vector
    float diffuseIntensity = max(0.0, dot(L,n));
    vec4 diffuseColor = U_DiffuseLightColor * U_DiffuseMaterial * diffuseIntensity;


    // specular, 采用 blinn-phone模型
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

    gl_FragColor = ambientColor + diffuseColor*attenuation + specularColor*attenuation;
}