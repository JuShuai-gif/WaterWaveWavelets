// DIR_NUM/4
const int NUM = 4;
// 8 * DIR_NUM
const int NUM_INTEGRATION_NODES = 128;

uniform sampler1D textureData;
uniform float profilePeriod;

// Define getAmpl for vertex shader
#if VERTEX_SHADER
layout(location = 1) in highp vec4 amplitude[NUM];

float Ampl(int i) {
    i = i % 16;
    return amplitude[i / 4][i % 4];
}
#endif

// Define getAmpl for fragment shader
#if FRAGMENT_SHADER
in highp vec4 ampl[NUM];

float Ampl(int i) {
    i = i % 16;
    return ampl[i / 4][i % 4];

}
#endif

const float tau = 6.28318530718;

float iAmpl(float angle/*in [0,2pi]*/) {
    float a = 16 * angle / tau + 16 - 0.5;
    int ia = int(floor(a));
    float w = a - ia;
    return (1 - w) * Ampl(ia % 16) + w * Ampl((ia + 1) % 16);
}

int seed = 40234324;

vec3 wavePosition(vec3 p) {

    vec3 result = vec3(0.0, 0.0, 0.0);

    const int N = NUM_INTEGRATION_NODES;
    float da = 1.0 / N;
    float dx = 16 * tau / N;
    for (float a = 0; a < 1; a += da) {

        float angle = a * tau;
        vec2 kdir = vec2(cos(angle), sin(angle));
        float kdir_x = dot(p.xy, kdir) + tau * sin(seed * a);
        float w = kdir_x / profilePeriod;

        vec4 tt = dx * iAmpl(angle) * texture(textureData, w);

        result.xy += kdir * tt.x;
        result.z += tt.y;
    }

    return result;
}

vec3 waveNormal(vec3 p) {

    vec3 tx = vec3(1.0, 0.0, 0.0);
    vec3 ty = vec3(0.0, 1.0, 0.0);

    const int N = NUM_INTEGRATION_NODES;
    float da = 1.0 / N;
    float dx = 16 * tau / N;
    for (float a = 0; a < 1; a += da) {

        float angle = a * tau;
        vec2 kdir = vec2(cos(angle), sin(angle));
        float kdir_x = dot(p.xy, kdir) + tau * sin(seed * a);
        float w = kdir_x / profilePeriod;

        vec4 tt = dx * iAmpl(angle) * texture(textureData, w);

        tx.xz += kdir.x * tt.zw;
        ty.yz += kdir.y * tt.zw;
    }

    return normalize(cross(tx, ty));
}


