#include <iostream>
#include "NetworkLoader.h"
#include "GeneticAlgorithm.h"
#include "RouteFinder.h"

int main()
{
	auto network = NetworkLoader::load("network.json");

	GeneticAlgorithm genAlg;

	RouteFinder genAlgRouteFinder(network, genAlg);
	const Stop* start = network.getStop(1); // Example stop ID
	const Stop* end = network.getStop(18);   // Example stop ID
	auto departureTime = NetworkLoader::parseTime("00:00");

	Path path = genAlgRouteFinder.findRoute(start, end, departureTime);
	std::cout << path << std::endl;

	return 0;
}
