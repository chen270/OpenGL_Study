#version 330 core

in vec3 V_Normal;
// in vec4 V_WorldPos;

// uniform vec4 U_DiffuseLightColor;
// uniform vec4 U_DiffuseMaterial; // 环境光反射的材质
uniform vec3 U_LightPos;

uniform vec4 U_AmbientLightColor;
uniform vec4 U_AmbientMaterial; // 环境光反射的材质
// uniform vec3 U_EyePos;

// uniform vec4 U_SpecularLightColor;
// uniform vec4 U_SpecularMaterial; // 环境光反射的材质

void main()
{
    // amibent 
    vec4 ambientColor = U_AmbientLightColor * U_AmbientMaterial; // 环境光

    // diffuse
    vec3 L = normalize(U_LightPos); // L vector
    vec3 n = normalize(V_Normal); // n vector
    float diffuseIntensity = max(0.0, dot(L,n));
    // vec4 diffuseColor = U_DiffuseLightColor * U_DiffuseMaterial * diffuseIntensity;
    vec4 cartoonColor;//= vec4(0.8, 0.8, 0.8, 1.0);
    if(diffuseIntensity > 0.5)
    {
        cartoonColor = vec4(0.8, 0.8, 0.8, 1.0);
    }
    else
    {
        cartoonColor = vec4(0.4, 0.4, 0.4, 1.0);
    }

    // specular, -L 表示光源指向物体，即入射光线
    // vec3 reflectDir = reflect(-L, n);
    // reflectDir = normalize(reflectDir);
    // // vec4 worldPos = M * vec4(pos, 1); // 世界坐标系
    // vec3 viewDir = U_EyePos - V_WorldPos.xyz;
    // viewDir = normalize(viewDir);
    // vec4 specularColor = U_SpecularLightColor * U_SpecularMaterial * pow(max(0.0, dot(viewDir, reflectDir)), 128.0);

    gl_FragColor = ambientColor + cartoonColor;
}