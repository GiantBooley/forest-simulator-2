#version 330 core

in vec2 texCoord;
in vec3 pos;
in float tileHealth;
in vec3 lightColor;

out vec4 FragColor;

uniform sampler2D texture1;
uniform float time;

vec3 lerp(vec3 a, vec3 b, float t) {
	return (b - a) * t + a;
}
void main() {
	vec4 color = texture2D(texture1, texCoord);
	color.rgb = lerp(color.rgb, vec3(0.25, 0.25, 0.25), pos.z / -3.);
	color.rgb *= lightColor;
	FragColor = color;
}