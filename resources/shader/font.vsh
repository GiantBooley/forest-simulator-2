#version 330 core

uniform mat4 MVP;

in vec2 vTexCoord;
in vec3 vPos;

out vec2 texCoord;
out vec3 pos;

void main() {
	vec4 position = vec4(vPos, 1.0);
	gl_Position = position;
	texCoord = vTexCoord / 32.;
	texCoord.y = 1. - texCoord.y;
	pos = vPos;
}
