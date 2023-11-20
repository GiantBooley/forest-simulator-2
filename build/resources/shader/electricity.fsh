#version 330 core

in vec2 texCoord;
in vec3 pos;

out vec4 FragColor;

uniform sampler2D texture1;

void main() {
	vec4 color = vec4(0.8, 0.8, 1., 0.8);
	FragColor = color;
}