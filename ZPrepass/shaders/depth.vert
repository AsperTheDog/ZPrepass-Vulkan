#version 450

layout( push_constant ) uniform constants
{
	mat4 mvpMat;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

void main() 
{
	gl_Position = mvpMat * vec4(inPosition, 1.0);
}