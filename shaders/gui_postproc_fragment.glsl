#version 420 core
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D guiTextureColor;

void main()
{ 
    vec2 offset = vec2(0.0, 0.0) / textureSize(guiTextureColor, 0);
    FragColor = texture(guiTextureColor, TexCoords + offset);

}