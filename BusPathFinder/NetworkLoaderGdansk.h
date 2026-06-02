#pragma once

#include <string>
#include <chrono>

#include "Network.h"

class NetworkLoaderGdansk
{
public:
    static Network load(
        const std::string& gtfsDirectory,
        const std::string& targetDate
    );
};