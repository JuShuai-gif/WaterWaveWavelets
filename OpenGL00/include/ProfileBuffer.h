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
		Ԥ�ȼ������Ƶ�׵������ļ�������

		��ʽ21

		spectrum: ����Ƶ��A������zeta=log2(wavelength),�����ز��ܶ�
		time: Ԥ�ȼ������ʱ��������ļ�
		zeta_min: �����½磬zeta_min = log2(minimal * wavelength)
		zeta_max: �����Ͻ磬zeta_max = log2(maximal * wavelength)
		resolution: �ֱ��ʵĽڵ���
		periodicity: ���պ���������ȷ��Ϊ periodicity * pow(2,zeta_max)
		integration_nodes: ���ֽڵ������
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
		ͨ����Ԥ�ȼ�������ݽ������Բ�ֵ������ p �������
		p ����λ�ã�ͨ�� p = dot(position,wavedirection)
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
		// ������ȵ�ɫɢ��ϵ
		// https://en.wikipedia.org/wiki/Dispersion_(water_waves)
		float dispersionRelation(float k)const 
		{
			constexpr float g = 9.81;
			return sqrt(k * g);
		}

		// ������ https://en.wikipedia.org/wiki/Dispersion_(water_waves)
		/*  ��������ֵ�����飺
		1. ˮƽλ��ƫ��
		2. ��ֱλ��ƫ��
		3. ˮƽƫ�Ƶ�λ�õ���
		4. ��ֱƫ�Ƶ�λ�õ���
		*/
		std::array<float, 4> gerstner_wave(float phase, float knum)const 
		{
			float s = sin(phase);
			float c = cos(phase);
			return std::array<float, 4>{-s, c, -knum * c, -knum * s};
		}

		/*
		bubic_bump �ǻ��� p_0 �ĺ�������
		https://en.wikipedia.org/wiki/Dispersion_(water_waves)
		�Ƶ��õ�
		*/

		float cubic_bump(float x)const 
		{
			if (abs(x) >= 1)
				return 0.0f;
			else
				return x * x * (2 * abs(x) - 3) + 1;
		}

	public:
		// ����
		float m_period;
		// ����
		std::vector<std::array<float, 4>> m_data;
	};
}