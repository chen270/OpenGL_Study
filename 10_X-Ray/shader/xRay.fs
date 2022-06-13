#version 330 core

in vec3 V_Normal;
in vec4 V_WorldPos;

uniform vec3 U_EyePos;

void main()
{
    vec3 L = U_EyePos.xyz - V_WorldPos.xyz;
    L = normalize(L);
    vec3 n = normalize(V_Normal); // n vector
    float cosTheta = dot(L,n);
    float alpha = 0.0;
    if (cosTheta > 0.0)
    {
        alpha = 1 - cosTheta;
    }

    gl_FragColor = vec4(0.1, 0.1, 0.1, alpha);
}