#version 450

layout( push_constant ) uniform constantsColor 
{
    layout(offset = 64) vec3 color;
};

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 diffuseFinal = color * clamp(dot(vec3(1.0, 1.0, 0.0), normalize(fragNormal)) * 1.0, 0, 1);
    outColor = vec4(color * 0.05 + diffuseFinal, 1.0);
}