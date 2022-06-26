#version 330 core

in vec2 V_Texcoord;
uniform sampler2D U_BaseTexture;
uniform sampler2D U_BlendTexture;

void main()
{
    vec4 blendColor = texture2D(U_BlendTexture, V_Texcoord);
    vec4 baseColor = texture2D(U_BaseTexture, V_Texcoord);

    // 该算法等于 opengl: GL_ONE_MINUS_SRC_ALPHA
    gl_FragColor = blendColor * blendColor.a + baseColor * (1 - blendColor.a);

    // gl_FragColor = texture2D(U_BaseTexture, V_Texcoord) + texture2D(U_BlendTexture, V_Texcoord);
    // gl_FragColor=texture2D(U_MainTexture,V_Texcoord)+texture(U_HDRTexture,V_Texcoord);
}