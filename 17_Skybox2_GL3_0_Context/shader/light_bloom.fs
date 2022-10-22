#version 330 core

void main()
{
    // gl_FragData[0] = vec4(1.0)*1.0;
    gl_FragData[1] = vec4(1.0)*2.0; // 赋值到 color buffer1 hdr上
}