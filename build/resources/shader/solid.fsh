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
	if (color.a == 0.) discard;
	color.rgb = lerp(color.rgb, vec3(0.75, 0.75, 0.75), clamp(1. - pow(0.95, max(-pos.z, 0.)), 0., 1.));
	color.rgb *= lightColor;
	FragColor = color;
}