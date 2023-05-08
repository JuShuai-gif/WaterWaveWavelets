#include <iostream>
#include "../include/Integration.h"
#include "../include/ArrayAlgebra.h"
#include "utils/StringTools.h"
#include "utils/Timing.h"
#include "Test.h"

int main() {

	int N = 1<<10;
	std::vector<int> v;
	Utils::Timing::printAverageTimes();
	START_TIMING("myTimer");


	for (int i = 0; i < N; i++)
	{
		v.push_back(i);
	}
	std::cout << Test::getID() << std::endl;
	return 0;
}
