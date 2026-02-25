#version 330

layout(location=0) in vec3 vertexPos;
layout(location=1) in vec4 color;
layout(location=2) in mat4 modelMatrix;

out vec4 fragmentColor;

uniform mat4 perspective;

void main()
{
    gl_Position = perspective * modelMatrix * vec4(vertexPos, 1.0);
    fragmentColor = color;
}
    

