#version 410 core

//layout (location = 0) in vec3 aPos;
layout(location = 0) in highp vec3 position;
layout(location = 1) in highp vec4 amplitude[4];

// DIR_NUM/4
const int NUM = 4;
// 8 * DIR_NUM
const int NUM_INTEGRATION_NODES = 128;

uniform sampler1D textureData;
uniform float profilePeriod;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

/*** WaterSurface�еľ��� ***/
//uniform highp mat4 transformationMatrix;
//uniform highp mat4 projectionMatrix;
uniform mediump mat3 normalMatrix;
uniform highp vec3 light;
uniform float time;


out mediump vec3 transformedNormal;
out highp vec3 lightDirection;
out highp vec3 cameraDirection;
out highp vec3 pos;
out highp vec4 ampl[NUM];

// ��ȡ����
float Ampl(int i){
  i = i%16;
  return amplitude[i/4][i%4];
}

// 2��Բ����
const float tau = 6.28318530718;

// 
float iAmpl( float angle/*in [0,tau]*/){
  float a = 16*angle/tau + 16 - 0.5;
  int ia = int(floor(a));
  float w = a-ia;
  return (1-w)*Ampl(ia%16)+w*Ampl((ia+1)%16);
}

// �������
int seed = 40234324;

// ����λ��
vec3 wavePosition(vec3 p){

  vec3 result = vec3(0.0,0.0,0.0);

  const int N = NUM_INTEGRATION_NODES;
  float da = 1.0/N;
  float dx = 16*tau/N;
  for(float a = 0;a<1;a+=da){

    float angle = a*tau;
    vec2 kdir = vec2(cos(angle),sin(angle));
    float kdir_x = dot(p.xy,kdir)+tau*sin(seed*a);
    float w = kdir_x/profilePeriod;

    vec4 tt = dx*iAmpl(angle)*texture(textureData,w);

    result.xy += kdir*tt.x;
    result.z += tt.y;
  }

  return result;
}



void main()
{
    // �������ת��
    highp vec3 p = position.xyz;
    // ��ȡλ��
    p += wavePosition(p);
    // ԭʼ����
    vec3 normal = vec3(0.0,0.0,1.0);

    /* �任�������� */
    highp vec4 transformedPosition4 = model*vec4(p.x,p.y,p.z,1);
    highp vec3 transformedPosition = transformedPosition4.xyz/transformedPosition4.w;

    /* �任������ */
    transformedNormal = normalMatrix*normal;

    /* �ƹⷽ�� */
    lightDirection = normalize(light - transformedPosition);

    /* ��������� */
    cameraDirection = -transformedPosition;

    /* �任λ�� */
    gl_Position = projection * view * transformedPosition4;

    pos = position.xyz;

    /* ���ݷ�����Ƭ����ɫ�� */
    for(int i=0;i<NUM;i++){
        ampl[i] = amplitude[i];
    }
}
