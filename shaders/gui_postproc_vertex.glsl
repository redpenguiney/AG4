#version 330 core
layout (location = 0) in vec3 vertexPos;
layout (location = 2) in vec2 textureXY;

out vec2 TexCoords;

void main()
{
    gl_Position = vec4(vertexPos.x, vertexPos.y, 0.0, 1.0); 
    TexCoords = textureXY;
}  