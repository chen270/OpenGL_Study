#version 330 core

in vec2 V_Texcoord;
uniform sampler2D U_MainTexture;//纹理填充

void main()
{
    vec4 color = texture2D(U_MainTexture,V_Texcoord);
    // if (color.r > 2.0)
    //     discard;
    gl_FragColor=texture2D(U_MainTexture,V_Texcoord);
}