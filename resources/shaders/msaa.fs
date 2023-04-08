#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2DMS screenTexture;

void main()
{
    ivec2 viewPortDim = ivec2(1600, 900);
    ivec2 coord = ivec2(viewPortDim * TexCoords);

    vec3 sample0 = vec3(texelFetch(screenTexture, coord, 0));
    vec3 sample1 = vec3(texelFetch(screenTexture, coord, 1));
    vec3 sample2 = vec3(texelFetch(screenTexture, coord, 2));
    vec3 sample3 = vec3(texelFetch(screenTexture, coord, 3));

    vec3 color = 0.25 * (sample0 + sample1 + sample2 + sample3);
    FragColor = vec4(vec3(color), 1.0);
}