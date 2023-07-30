#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in float id; 

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform sampler1D texture1;

out vec4 color;

void main()
{
/*
	vec4 amplitudeData = vec4(0.0, 0.0, 0.0, 0.0);
	
    for(float i = 0;i<4;i++){
		amplitudeData += texture(texture1, id);
    }
	
	//vec4 amplitudeData = texture(texture1, textureCoord);

	color = amplitudeData;
*/
	//color = vec4(id,0,0,1);

	color = texture(texture1, 0.8f);

	gl_Position = projection * view * model * vec4(aPos, 1.0);
}