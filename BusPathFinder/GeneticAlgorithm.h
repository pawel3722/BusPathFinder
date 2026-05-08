#pragma once
#include "IAlgorithm.h"

class GeneticAlgorithm : public IAlgorithm
{
	public:
		Path findPath(const Network& network, const Stop* start, const Stop* end, std::chrono::minutes departureTime) override;
};