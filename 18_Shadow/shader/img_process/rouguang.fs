#version 330 core

in vec2 V_Texcoord;
uniform sampler2D U_BaseTexture;
uniform sampler2D U_BlendTexture;

void main()
{
    vec4 blendColor = texture2D(U_BlendTexture, V_Texcoord);
    vec4 baseColor = texture2D(U_BaseTexture, V_Texcoord);

    // 柔光
    gl_FragColor = 2.0 * baseColor*blendColor + baseColor*baseColor-2.0*baseColor*baseColor*blendColor;
}