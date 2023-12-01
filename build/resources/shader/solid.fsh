#version 330 core

in vec2 texCoord;
in vec3 pos;
in float tileHealth;
in vec3 lightColor;

out vec4 FragColor;

uniform sampler2D texture1;
uniform float time;

float lerp(float a, float b, float t) {
	return (b - a) * t + a;
}
void main() {
	vec4 color = texture2D(texture1, texCoord);
	color.rgb *= lightColor;
	color.a = lerp(color.a, 1.0, 1.0 - max(lightColor.b, max(lightColor.r, lightColor.g)));
	FragColor = color;
}