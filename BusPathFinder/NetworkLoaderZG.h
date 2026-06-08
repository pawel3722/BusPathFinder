#pragma once

#include <string>
#include <chrono>

#include "Network.h"

class NetworkLoaderZG
{
public:
    static Network load(
        const std::string& gtfsDirectory
    );
};