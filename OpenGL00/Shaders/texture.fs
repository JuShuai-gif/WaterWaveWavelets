#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

// texture sampler
uniform sampler1D texture1;
uniform sampler1D texture2;

void main()
{
	FragColor = texture(texture1, 0.75) + texture(texture2, 0.75);
}