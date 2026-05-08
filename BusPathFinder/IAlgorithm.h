#pragma once
#include "Network.h"
#include "Path.h"

class IAlgorithm
{
public:
	virtual Path findPath(const Network& network, const Stop* start, const Stop* end, std::chrono::minutes departureTime) = 0;
};