#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 id; 

// DIR_NUM / 4 = 16 / 4
const int NUM = 4;
// 8 * DIR_NUM = 128
//const int NUM_INTEGRATION_NODES = 128;

uniform sampler1D textureData;
// 轮廓周期
uniform float profilePeriod;

// 原来是传递的数组，现在传递贴图（数组太大了不让传）
uniform sampler1D amplitudeData;

vec4 amplitude[4];
void SetAmplitude(){
	amplitude[0] = texture(amplitudeData,id.x);
	amplitude[1] = texture(amplitudeData,id.y);
	amplitude[2] = texture(amplitudeData,id.z);
	amplitude[3] = texture(amplitudeData,id.w);
}


float Ampl(int i){
	i = i % 16;
	return amplitude[i/4][i%4];
}

const float tau = 6.28318530718;

float iAmpl(float angle){
	float a = 16 * angle / tau + 16.0f - 0.5f;
	int ia = int(floor(a));
	float w = a - ia;
	return (1 - w) * Ampl(ia % 16) + w * Ampl((ia + 1) % 16);
}

int seed = 40234324;

vec3 wavePosition(vec3 p){
	vec3 result = vec3(0.0, 0.0, 0.0);
	
	const int N = 128;
	float da = 1.0 / N;
	float dx = 16 * tau / N;
	for(float a = 0; a < 1; a+=da){
		float angle = a * tau;
		vec2 kdir = vec2(cos(angle),sin(angle));
		float kdir_x = dot(p.xy,kdir) + tau * sin(seed * a);
		float w = kdir_x / profilePeriod;

		vec4 tt = dx*iAmpl(angle)*texture(textureData,w);

		result.xy += kdir*tt.x;
		result.z += tt.y;
	}
	return result;
}

vec3 waveNormal(vec3 p){
	vec3 tx = vec3(1.0, 0.0, 0.0);
	vec3 ty = vec3(0.0, 1.0, 0.0);

	const int N = 128;
	float da = 1.0 / N;
	float dx = 16 * tau / N;
	for(float a = 0;a < 1; a+=da){
		float angle = a * tau;
		vec2 kdir = vec2(cos(angle),sin(angle));
		float kdir_x = dot(p.xy,kdir) + tau * sin(seed * a);
		float w = kdir_x / profilePeriod;

		vec4 tt = dx * iAmpl(angle) * texture(textureData,w);

		tx.xz +=kdir.x * tt.zw;
		ty.yz +=kdir.y * tt.zw;
	}
	return normalize(cross(tx,ty));
}


uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 light;
uniform float time;

out vec3 transformedNormal;
out vec3 lightDirection;
out vec3 cameraDirection;
out vec3 pos;
out vec4 ampl[NUM];
out vec4 color;

void main()
{
	SetAmplitude();

	vec3 p = aPos;
	//p += wavePosition(p);
	vec3 normal = vec3(0.0f, 0.0f, 1.0f);

	// 变换顶点坐标
	highp vec4 transformedPosition4 = model *vec4(p.x,p.y,p.z,1);
	highp vec3 transformedPosition = transformedPosition4.xyz/transformedPosition4.w;

	// 变换法线
	transformedNormal = (model* vec4(normal,1.0f)).xyz;

	// 灯光方向
	lightDirection = normalize(light - transformedPosition);

	// 相机方向
	cameraDirection = -transformedPosition;

	// 变换位置
	gl_Position = projection* view *transformedPosition4;

	pos = aPos.xyz;

	/* Pass amplitudes to fragment shader */
	for(int i=0;i<NUM;i++){
	  ampl[i] = amplitude[i];
	}

	color = (amplitude[0] + amplitude[1] + amplitude[2] + amplitude[3]) / 4;
	//color = texture(textureData, id) + texture(amplitudeData, id);
	
	//gl_Position = projection * view * model * vec4(aPos, 1.0);
}