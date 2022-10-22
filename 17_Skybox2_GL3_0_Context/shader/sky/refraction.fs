#version 330 core

uniform samplerCube U_MainTexture; // 立方体贴图的纹理采样器
// in vec3 V_Texcoord;
in vec4 V_WorldPos;
in vec3 V_Normal;


void main()
{
    float ratio = 1.00 / 1.52;
    vec3 eyeVec = normalize(V_WorldPos.xyz - vec3(0.0));
    vec3 n = normalize(V_Normal);
    vec3 r = refract(eyeVec, n, ratio);
    vec4 color = texture(U_MainTexture, r);
    // if(color.r > 2.0)
        gl_FragColor = vec4(color.xyz, 1.0);
    // else
        // gl_FragColor = vec4(1.0);
}