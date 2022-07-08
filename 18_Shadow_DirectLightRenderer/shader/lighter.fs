#version 330 core

in vec2 V_Texcoord;
uniform sampler2D U_BaseTexture;
uniform sampler2D U_BlendTexture;

void main()
{
    vec4 blendColor = texture2D(U_BlendTexture, V_Texcoord);
    vec4 baseColor = texture2D(U_BaseTexture, V_Texcoord);

    // 单张
    // gl_FragColor = vec4(1.5*baseColor.rgb, baseColor.a); // 变亮
    // gl_FragColor = vec4(baseColor.rgb, baseColor.a  * 0.5); // 变暗

    // 多张
    gl_FragColor = max(blendColor, baseColor); // 变亮
    // gl_FragColor = min(blendColor, baseColor); // 变暗
}