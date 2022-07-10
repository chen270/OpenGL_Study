attribute vec3 pos;
attribute vec2 texcoord;
attribute vec3 normal;

uniform mat4 M;
uniform mat4 P;
uniform mat4 V;
uniform mat4 NM;

varying vec3 V_Normal;
varying vec3 V_WorldPos;

void main()
{
	V_Normal=mat3(NM)*normal;
	vec4 worldPos=M*vec4(pos,1.0);
	V_WorldPos=worldPos.xyz;

	gl_Position=P*V*worldPos;
}