#pragma once
#include "IAlgorithm.h"

class RouteFinder
{
	IAlgorithm& algorithm;
	Network& network;
public:
	RouteFinder(Network& net, IAlgorithm& alg) : network(net), algorithm(alg) {}

	Path findRoute(const Stop* start, const Stop* end, std::chrono::minutes departureTime)
	{
		return algorithm.findPath(network, start, end, departureTime);
	}
};