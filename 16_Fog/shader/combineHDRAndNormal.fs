#version 330 core

in vec2 V_Texcoord;
uniform sampler2D U_MainTexture;
uniform sampler2D U_HDRTexture;

void main()
{
    gl_FragColor = texture2D(U_MainTexture, V_Texcoord) + texture2D(U_HDRTexture, V_Texcoord);
    // gl_FragColor=texture2D(U_MainTexture,V_Texcoord)+texture(U_HDRTexture,V_Texcoord);
}