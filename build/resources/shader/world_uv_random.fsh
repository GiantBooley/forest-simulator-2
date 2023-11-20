#version 330 core

in vec2 texCoord;
in vec3 pos;

out vec4 FragColor;

uniform sampler2D texture1;

float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}
vec2 rotate(vec2 it, float r) {
	return vec2(
		it.x * cos(r) - it.y * sin(r),
		it.y * cos(r) + it.x * sin(r)
	);
}
vec2 roundToPlace(vec2 it, float place) {
	return vec2(floor(it.x / place) * place, floor(it.y / place) * place);
}

void main() {
	vec4 color = texture2D(texture1, rotate(pos.xz, rand(roundToPlace(pos.xz, 0.1))));
	FragColor = color;
}