#version 330 core
//# 片段着色器所做的是计算像素最后的颜色输出，颜色每个分量的强度设置在0.0到1.0之间。
varying vec4 V_Color;

void main()
{
    gl_FragColor=V_Color;
}