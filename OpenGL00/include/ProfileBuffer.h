#pragma once
#include <array>
#include <vector>
#include "../include/Math.h"

namespace WaterWavelets 
{
	class ProfileBuffer 
	{
	public:

		/*
		预先计算给定频谱的配置文件缓冲区

		公式21

		spectrum: 参数频谱A，接受zeta=log2(wavelength),并返回波密度
		time: 预先计算给定时间的配置文件
		zeta_min: 积分下界，zeta_min = log2(minimal * wavelength)
		zeta_max: 积分上界，zeta_max = log2(maximal * wavelength)
		resolution: 分辨率的节点数
		periodicity: 最终函数的周期确定为 periodicity * pow(2,zeta_max)
		integration_nodes: 积分节点的数量
		*/
		template<typename Spectrum>
		void precompute(Spectrum& spectrum,float time,float zeta_min,
			float zeta_max,int resolution = 4096,int periodicity = 2,
			int integration_nodes = 100) 
		{
			m_data.resize(resolution);
			m_period = periodicity * pow(2, zeta_max);
#pragma omp parallel for
			for (int i = 0; i < resolution; ++i)
			{
				constexpr float tau = 6.28318530718;
				float p = (i * m_period) / resolution;
				m_data[i] = integrate(integration_nodes, zeta_min, zeta_max, [&](float zeta)
					{
				float waveLength = pow(2,zeta);
				float waveNumber = tau / waveLength;
				float phasel = waveNumber * p - dispersionRelation(waveNumber) * time;

				float phase2 = waveNumber * (p - m_period) - dispersionRelation(waveNumber) * time;
				float weight1 = p / m_period;
				float weight2 = 1 - weight1;
				return waveLength * spectrum(zeta) * (cubic_bump(weight1) * gerstner_wave(phasel, waveNumber)
					+ cubic_bump(weight2) * gerstner_wave(phase2, waveNumber));
					});
			}
		}
		/*
		通过对预先计算的数据进行线性插值来评估 p 点的轮廓
		p 评估位置，通常 p = dot(position,wavedirection)
		*/
		std::array<float, 4> operator()(float p)const 
		{
			const int N = m_data.size();
			auto extended_buffer = [=](int i)->std::array<float, 4>{
				return m_data[pos_modulo(i, N)];
			};

			auto interpolated_buffer = LinearInterpolation(extended_buffer);

			return interpolated_buffer(N * p / m_period);
		}

	private:
		// 无限深度的色散关系
		// https://en.wikipedia.org/wiki/Dispersion_(water_waves)
		float dispersionRelation(float k)const 
		{
			constexpr float g = 9.81;
			return sqrt(k * g);
		}

		// 波生成 https://en.wikipedia.org/wiki/Dispersion_(water_waves)
		/*  返回以下值的数组：
		1. 水平位置偏移
		2. 垂直位置偏移
		3. 水平偏移的位置导数
		4. 垂直偏移的位置导数
		*/
		std::array<float, 4> gerstner_wave(float phase, float knum)const 
		{
			float s = sin(phase);
			float c = cos(phase);
			return std::array<float, 4>{-s, c, -knum * c, -knum * s};
		}

		/*
		bubic_bump 是基于 p_0 的函数，从
		https://en.wikipedia.org/wiki/Dispersion_(water_waves)
		推导得到
		*/

		float cubic_bump(float x)const 
		{
			if (abs(x) >= 1)
				return 0.0f;
			else
				return x * x * (2 * abs(x) - 3) + 1;
		}

	public:
		// 周期
		float m_period;
		// 数据
		std::vector<std::array<float, 4>> m_data;
	};
}