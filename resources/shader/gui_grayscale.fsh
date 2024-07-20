#version 330 core

in vec2 texCoord;
in vec3 pos;
in float tileHealth;
in vec3 lightColor;

out vec4 FragColor;

uniform sampler2D texture1;
uniform float time;

float grayscale(vec3 color) {
	return (color.r + color.g + color.b) / 3.;
}

void main() {
	vec4 color = texture2D(texture1, texCoord);
	color.rgb = vec3(grayscale(color.rgb));
	FragColor = color;
}