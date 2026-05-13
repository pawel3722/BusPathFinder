#include <iostream>
#include <windows.h>
#include <string>
#include "NetworkLoader.h"
#include "GeneticAlgorithm.h"
#include "RouteFinder.h"
#include "Functions.h" 

int main()
{
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
    #endif



    auto network = NetworkLoader::load("gd_stops.json","gd_trips.json","gd_stop_times.json");

    GeneticAlgorithm genAlg;
    RouteFinder genAlgRouteFinder(network, genAlg);

    int startId = 0;
    int endId = 0;
    std::string timeStr = "00:00";
    while (true)
    {

        const Stop* start = nullptr;
        const Stop* end = nullptr;
        std::chrono::minutes departureTime;

        std::cout << "Start stop ID: ";
        std::cin >> startId;

        if (startId == 0)
        {
            start = network.getRandomStop();
            end = network.getRandomStop();
            departureTime = randomTime();

            std::cout << "FROM: " << start->getName() << " TO: " << end->getName() << " AT: " << formatTime(departureTime) << std::endl;
        }
        else
        {
            std::cout << "End stop ID: ";
            std::cin >> endId;

            std::cout << "Departure time (HH:MM): ";
            std::cin >> timeStr;

            start = network.getStop(startId);
            end = network.getStop(endId);

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


            try {
                departureTime = parseTime(timeStr);
            }
            catch (std::exception ex)
            {
                departureTime = parseTime("00:00");
            }
        }

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
