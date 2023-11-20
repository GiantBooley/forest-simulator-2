#version 330 core

in vec2 texCoord;
in vec3 pos;

out vec4 FragColor;

uniform sampler2D texture1;

void main() {
	vec4 color = texture2D(texture1, texCoord);
	if (color.a == 0.) discard;
	FragColor = color;
}