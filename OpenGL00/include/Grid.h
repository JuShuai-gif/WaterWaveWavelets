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
		// 数据
		std::vector<Real> m_data;
		
		/*
		里面分别存放四个维度的数据，它们的长度不一样
		第一个是x坐标的长度   100
		第二个是y坐标的长度	  100
		第三个是theta的范围   16
		第四个是k              
		*/
		std::array<int, 4> m_dimensions;
	};

}
