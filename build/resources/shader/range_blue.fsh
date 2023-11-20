#version 330 core

in vec2 texCoord;
in vec3 pos;

out vec4 FragColor;

uniform sampler2D texture1;

bool circle(vec2 texcoord, vec2 pos, float r) {
	return distance(texcoord, pos) < r;
}
void main() {
	if (!circle(texCoord, vec2(0.5, 0.5), 0.5)) {
		discard;
	}
	vec4 color = vec4(0.5, 0.5, 0.9, 0.8);
	FragColor = color;
}