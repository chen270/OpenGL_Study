# version 330 core

in vec3 pos;
out vec2 V_Texcoord;

in vec2 texcoord;

void main()
{
    vec4 worldPos = vec4(pos, 1.0);
    // V_Texcoord = pos.xy + vec2(0.5, 0.5);
    V_Texcoord = texcoord;
    worldPos.x *= 2.0;
    worldPos.y *= 2.0;
    gl_Position = worldPos;
}