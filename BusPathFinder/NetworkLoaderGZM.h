#pragma once

#include <string>
#include <chrono>

#include "Network.h"

class NetworkLoaderGZM
{
public:
    static Network load(
        const std::string& gtfsDirectory
    );
};