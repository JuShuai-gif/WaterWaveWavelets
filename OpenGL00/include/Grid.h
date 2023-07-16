#pragma once

#include "Global.h"
namespace WaterWavelets 
{
	class Grid
	{
	public:
		Grid();
		void resize(int n0, int n1, int n2, int n3);
		Real& operator()(int i0, int i1, int i2, int i3);

		Real const& operator()(int i0, int i1, int i2, int i3)const;

		int dimension(int dim)const;

	private:
		// ����
		std::vector<Real> m_data;
		// ά��
		std::array<int, 4> m_dimensions;
	};

}
