#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture1;
uniform float blendConst;

void main()
{
    FragColor = vec4(vec3(texture(texture1, TexCoords)), blendConst) * vec4(0.03,0.01,0.01,1);
}