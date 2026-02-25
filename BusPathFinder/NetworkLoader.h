#pragma once
#include <string>
#include "Network.h"

class NetworkLoader
{
public:
	static Network load(const std::string& filename);
	static std::chrono::minutes parseTime(const std::string& str);
};