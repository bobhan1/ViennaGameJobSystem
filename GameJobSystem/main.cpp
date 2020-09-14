
#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <functional>
#include <string>
#include <algorithm>
#include <chrono>
#include <glm.hpp>

#include "VEGameJobSystem2.h"

namespace coro {
	void test();
}

namespace func {
	void test();
}

namespace mixed {
	void test();
}


int main()
{
	using namespace vgjs;

	mixed::test();

	std::string t;
	std::cin >> t;

	vgjs::terminate();
	wait_for_termination();
}


