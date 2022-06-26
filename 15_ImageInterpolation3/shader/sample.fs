#version 330 core

varying vec2 V_Texcoord;
uniform sampler2D U_MainTexture;//纹理填充
uniform sampler2D U_Wood;//纹理填充

void main()
{
    // gl_FragColor=vec4(1.0);
//    gl_FragColor = texture2D(U_MainTexture,V_Texcoord) * texture2D(U_Wood,V_Texcoord);
    gl_FragColor=texture2D(U_MainTexture,V_Texcoord)*texture2D(U_Wood,V_Texcoord);
}