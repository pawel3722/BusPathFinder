#pragma once
#include <string>
#include "Network.h"

class NetworkLoader
{
public:
    static Network load(
        const std::string& stopsFile,
        const std::string& tripsFile,
        const std::string& stopTimesFile);
	static std::chrono::minutes parseTime(const std::string& str);
};