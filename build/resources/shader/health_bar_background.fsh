#version 330 core

in vec2 texCoord;
in vec3 pos;
in float tileHealth;
in vec3 lightColor;

out vec4 FragColor;

uniform sampler2D texture1;
uniform float time;

void main() {
	vec4 color = vec4(0., 0., 0., 1.);
	FragColor = color;
}