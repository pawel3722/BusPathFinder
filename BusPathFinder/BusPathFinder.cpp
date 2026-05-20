#include <iostream>
#include <windows.h>
#include <string>
#include <future>
#include "NetworkLoader.h"
#include "GeneticAlgorithm.h"
#include "RouteFinder.h"
#include "Functions.h" 
#include "ACOAlgorithm.h"

int main()
{
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
    #endif



    auto network = NetworkLoader::load("gd_stops.json","gd_trips.json","gd_stop_times.json");

    GeneticAlgorithm genAlg;
    ACOAlgorithm acoAlg;
    RouteFinder genAlgRouteFinder(network, genAlg);
    RouteFinder acoAlgRouteFinder(network, acoAlg);

    int startId = 0;
    int endId = 0;
    std::string timeStr = "00:00";
    while (true)
    {

        const Stop* start = nullptr;
        const Stop* end = nullptr;
        std::chrono::minutes departureTime;
        std::cout << "|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||QUERY||||||||||||||||||||" << std::endl;
        std::cout << "Start stop ID: ";
        std::cin >> startId;

        if (startId == 0)
        {
            start = network.getRandomStop();
            end = network.getRandomStop();
            departureTime = randomTime();

            std::cout << "FROM: " << start->getName() << " <" << start->getId() << "> TO: " << end->getName() << " <" << end->getId() << "> AT: " << formatTime(departureTime) << std::endl;
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
                startId = 0;
                continue;
            }

            if (!end)
            {
                std::cout << "Invalid end stop ID\n";
                startId = 0;
                continue;
            }


            try {
                departureTime = parseTime(timeStr);
            }
            catch (std::exception ex)
            {
                departureTime = parseTime("00:00");
            }
        }

        std::vector<Path> pathsGen;
        std::vector<Path> pathsAco;

        auto futureGen = std::async(std::launch::async, [&]()
            {
                auto res = genAlgRouteFinder.findRoute(
                    start,
                    end,
                    departureTime);
                std::cout << "GEN ready! " << std::endl;
                return res;
            });

        auto futureAco = std::async(std::launch::async, [&]()
            {
                auto res = acoAlgRouteFinder.findRoute(
                    start,
                    end,
                    departureTime);
                std::cout << "ACO ready! " << std::endl;
                return res;
            });

        // bariera — czekamy na oba wyniki
        try
        {
            pathsGen = futureGen.get();
        }
        catch (const std::exception& ex)
        {
            std::cout << "GEN exception: " << ex.what() << std::endl;
        }

        try
        {
            pathsAco = futureAco.get();
        }
        catch (const std::exception& ex)
        {
            std::cout << "ACO exception: " << ex.what() << std::endl;
        }

        std::cout << "++++++++++++++++++++++++++++++++++++GEN++++++++++++++++++++++++++++++++++++" << std::endl;

        for (const auto& path : pathsGen)
        {
            std::cout << path << std::endl;
        }

        std::cout << "++++++++++++++++++++++++++++++++++++ACO++++++++++++++++++++++++++++++++++++" << std::endl;

        for (const auto& path : pathsAco)
        {
            std::cout << path << std::endl;
        }
    }

    return 0;
}
