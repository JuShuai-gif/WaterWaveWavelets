#version 450 core
out vec4 FragColor;

in vec4 color;

void main()
{
	FragColor = color;
	//FragColor = vec4(1.0f,0.0f,0.0f,1.0f);
}