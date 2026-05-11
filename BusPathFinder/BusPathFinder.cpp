#include <iostream>
#include "NetworkLoader.h"
#include "GeneticAlgorithm.h"
#include "RouteFinder.h"

#include <string>


int main()
{
    auto network = NetworkLoader::load("network.json");

    GeneticAlgorithm genAlg;
    RouteFinder genAlgRouteFinder(network, genAlg);

    int startId = 1;
    int endId = 18;
    std::string timeStr = "00:00";

    while (true)
    {

        std::cout << "Start stop ID: ";
        std::cin >> startId;

        std::cout << "End stop ID: ";
        std::cin >> endId;

        std::cout << "Departure time (HH:MM): ";
        std::cin >> timeStr;

        const Stop* start = network.getStop(startId);
        const Stop* end = network.getStop(endId);

        if (!start)
        {
            std::cout << "Invalid start stop ID\n";
            return 1;
        }

        if (!end)
        {
            std::cout << "Invalid end stop ID\n";
            return 1;
        }

        auto departureTime = NetworkLoader::parseTime(timeStr);

        auto paths = genAlgRouteFinder.findRoute(
            start,
            end,
            departureTime
        );

        for (const auto& path : paths)
        {
            std::cout << path << std::endl;
        }
    }

    return 0;
}
