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
    if (U_LightPos.w == 0.0)
    {
        // direction light
        L = U_LightPos.xyz; // L vector
    }
    else
    {
        // model point -> light pos
        // point light / spot light
    }

    // diffuse
    L = normalize(L);
    vec3 n = normalize(V_Normal); // n vector
    float diffuseIntensity = max(0.0, dot(L,n));
    vec4 diffuseColor = U_DiffuseLightColor * U_DiffuseMaterial * diffuseIntensity;

    // specular, -L 表示光源指向物体，即入射光线
    vec3 reflectDir = reflect(-L, n);
    reflectDir = normalize(reflectDir);
    // vec4 worldPos = M * vec4(pos, 1); // 世界坐标系
    vec3 viewDir = U_EyePos - V_WorldPos.xyz;
    viewDir = normalize(viewDir);
    vec4 specularColor = U_SpecularLightColor * U_SpecularMaterial * pow(max(0.0, dot(viewDir, reflectDir)), 128.0);

    gl_FragColor = ambientColor + diffuseColor + specularColor;
}