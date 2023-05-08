#include <iostream>
#include "../include/Integration.h"
#include "../include/ArrayAlgebra.h"
#include "utils/StringTools.h"
#include "utils/Timing.h"
#include "Test.h"

//初始化那些宏定义变量

INIT_LOGGING
INIT_TIMING

int main() {

	int N = 1<<15;
	std::vector<int> v;
	Utils::Timing::printAverageTimes();

	Utils::Timing::startTiming("first time");
	for (int i = 0; i < N; i++)
	{
		v.push_back(i);
		v[i]*=12;
		v[i]+=15;
	}
	Utils::Timing::stopTiming();

	//std::cout << Test::getID() << std::endl;
	return 0;
}
