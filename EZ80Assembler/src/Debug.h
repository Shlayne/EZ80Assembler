#pragma once

#if CONFIG_DEBUG
	#include <iostream>
	#include <cassert>
	#define stdcout(x) std::cout << x
	#define stdcerr(x) std::cerr << x
	#define stdcin(x) std::cin >> x
#else
	#define stdcout(x)
	#define stdcerr(x)
	#define stdcin(x)
#endif
