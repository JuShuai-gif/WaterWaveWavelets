#include <iostream>
#include "../include/Integration.h"
#include "../include/ArrayAlgebra.h"
#include "utils/StringTools.h"
int main() {
	std::vector<std::string> tokens;
	Utils::StringTools::tokenize("hello, world!", tokens, ", ");
	for ( auto& it:tokens)
	{
		std::cout << it << std::endl;
	}
	return 0;
}
