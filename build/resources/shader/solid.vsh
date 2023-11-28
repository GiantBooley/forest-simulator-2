#version 330 core

uniform mat4 MVP;

in vec2 vTexCoord;
in vec3 vPos;

out vec2 texCoord;
out vec3 pos;

void main() {
	vec3 newPos = vPos;

	vec4 position = MVP * vec4(newPos, 1.0);
	gl_Position = position;
	texCoord = vTexCoord;
	pos = vPos;
}