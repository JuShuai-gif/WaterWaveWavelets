#version 410 core

layout(location = 1) in highp vec4 amplitude[4];

uniform lowp vec4 ambientColor;
uniform lowp vec4 color;
uniform mat3 normalMatrix;
uniform float time;
uniform sampler1D textureData;

uniform float profilePeriod;
//uniform float waveNumber;

in mediump vec3 transformedNormal;
in highp vec3 lightDirection;
in highp vec3 cameraDirection;
in highp vec3 pos;


out vec4 FragColor;

// 获取幅度
float Ampl(int i){
  i = i%16;
  return amplitude[i/4][i%4];
}

int seed = 40234324;
const float tau = 6.28318530718;
const int NUM_INTEGRATION_NODES = 128;

float iAmpl( float angle/*in [0,tau]*/){
  float a = 16*angle/tau + 16 - 0.5;
  int ia = int(floor(a));
  float w = a-ia;
  return (1-w)*Ampl(ia%16)+w*Ampl((ia+1)%16);
}
// 计算法线
vec3 waveNormal(vec3 p){

  vec3 tx = vec3(1.0,0.0,0.0);
  vec3 ty = vec3(0.0,1.0,0.0);  

  const int N = NUM_INTEGRATION_NODES;
  float da = 1.0/N;
  float dx = 16*tau/N;
  for(float a = 0;a<1;a+=da){

    float angle = a*tau;
    vec2 kdir = vec2(cos(angle),sin(angle));
    float kdir_x = dot(p.xy,kdir)+tau*sin(seed*a);
    float w = kdir_x/profilePeriod;

    vec4 tt = dx*iAmpl(angle)*texture(textureData,w);

    tx.xz += kdir.x*tt.zw;
    ty.yz += kdir.y*tt.zw;
  }

  return normalize(cross(tx,ty));
}

void main()
{
    // 读取法线
    highp vec3 normal = normalMatrix*waveNormal(pos);

    // 读取属性
    lowp vec4 finalAmbientColor = ambientColor;
    lowp vec4 finalDiffuseColor = color;
    lowp vec4 finalSpecularColor = vec4(1.0,1.0,1.0,1.0);
    lowp vec4 lightColor = vec4(1.0,1.0,1.0,1.0);
    
    // 赋值边界颜色
    if(pos.x < -50 || pos.x > 50 || pos.y<-50 || pos.y>50)
      finalDiffuseColor.rgb = vec3(0.6,0.6,0.6);

    /* 环境光 */
    FragColor = finalAmbientColor;
    // 归一化法线
    mediump vec3 normalizedTransformedNormal = normalize(normal);
    // 归一化灯光方向
    highp vec3 normalizedLightDirection = normalize(lightDirection);

    /* 漫反射颜色 */
    // 定义强度
    lowp float intensity = max(0.0, dot(normalizedTransformedNormal, normalizedLightDirection));
    // 最终颜色
    FragColor += finalDiffuseColor*lightColor*intensity;

    /* 增加高光 */
    if(intensity > 0.001) {
      vec3 ref = reflect(normalize(cameraDirection),normalizedTransformedNormal);
      highp float sky = max(0,pow((1-abs(dot(normalize(cameraDirection),normalizedTransformedNormal))),1)*sin(20*ref.x)*sin(20*ref.y)*sin(20*ref.z));
      highp vec3 reflection = reflect(-normalizedLightDirection, normalizedTransformedNormal);
      highp float shininess = 80.0;
      mediump float specularity = pow(max(0.0, dot(normalize(cameraDirection), reflection)), shininess);
      FragColor += finalSpecularColor*specularity + sky*vec4(1,1,1,1);
    }
    //FragColor = vec4(1,0,0,1); // set all 4 vector values to 1.0
}