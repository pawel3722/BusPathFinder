#include <iostream>
#include <windows.h>
#include "NetworkLoader.h"
#include "GeneticAlgorithm.h"
#include "RouteFinder.h"
#include <random>
#include <string>

static std::mt19937 rng(std::random_device{}());

static int randomInt(int a, int b)
{
    if (a == b)
        return a;
    if (a > b)
        std::swap(a, b);
    std::uniform_int_distribution<int> dist(a, b);
    return dist(rng);
}


int main()
{
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
    #endif



    auto network = NetworkLoader::load("gd_stops.json","gd_trips.json","gd_stop_times.json");

    GeneticAlgorithm genAlg;
    RouteFinder genAlgRouteFinder(network, genAlg);

    int startId = 1461;
    int endId = 114;
    std::string timeStr = "17:00";

    while (true)
    {

        /*std::cout << "Start stop ID: ";
        std::cin >> startId;

        std::cout << "End stop ID: ";
        std::cin >> endId;

        std::cout << "Departure time (HH:MM): ";
        std::cin >> timeStr;*/

        if (startId == 0)
            startId = randomInt(1, 21);
        if (endId == 0)
            endId = randomInt(1, 21);

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

        std::chrono::minutes departureTime;
        try {
            departureTime = NetworkLoader::parseTime(timeStr);
        }
        catch (std::exception ex)
        {
            departureTime = NetworkLoader::parseTime("00:00");
        }

        auto paths = genAlgRouteFinder.findRoute(
            start,
            end,
            departureTime
        );

        //for (const auto& path : paths)
        //{
        //    std::cout << path << std::endl;
        //}
        std::cout << "koniec\n";
        std::string x;
        std::cin >> x;
    }

    return 0;
}
