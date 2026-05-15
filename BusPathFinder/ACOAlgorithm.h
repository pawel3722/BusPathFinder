#pragma once
#include "IAlgorithm.h"


class ACOAlgorithm : public IAlgorithm
{
	std::vector<Path> findPath(const Network& network, const Stop* start, const Stop* end, std::chrono::minutes departureTime);    
};