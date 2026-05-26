#include <iostream>
#include <fstream>
#include <sstream>
#define NOMINMAX
#include <windows.h>
#include <string>
#include <future>
#include "NetworkLoader.h"
#include "GeneticAlgorithm.h"
#include "RouteFinder.h"
#include "Functions.h" 
#include "ACOAlgorithm.h"
#include "PSOAlgorithm.h"
#include "Result.h"

int main()
{
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
    #endif



    auto network = NetworkLoader::load("gd_stops.json","gd_trips.json","gd_stop_times.json");

    GeneticAlgorithm genAlg;
    ACOAlgorithm acoAlg;
    PSOAlgorithm psoAlg;
    RouteFinder genAlgRouteFinder(network, genAlg);
    RouteFinder acoAlgRouteFinder(network, acoAlg);
    RouteFinder psoAlgRouteFinder(network, psoAlg);

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
        std::vector<Path> pathsPso;

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
        auto futurePso = std::async(std::launch::async, [&]()
            {
                auto res = psoAlgRouteFinder.findRoute(
                    start,
                    end,
                    departureTime);
                std::cout << "PSO ready! " << std::endl;
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

        try
        {
            pathsPso = futurePso.get();
        }
        catch (const std::exception& ex)
        {
            std::cout << "PSO exception: " << ex.what() << std::endl;
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

        std::cout << "++++++++++++++++++++++++++++++++++++PSO++++++++++++++++++++++++++++++++++++" << std::endl;

        for (const auto& path : pathsPso)
        {
            std::cout << path << std::endl;
        }
    }

    return 0;
}

int main2(int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

	std::string stopsFile = "gd_stops.json";
	std::string tripsFile = "gd_trips.json";
	std::string stopTimesFile = "gd_stop_times.json";
	std::string outputPath = "output.txt";
	std::string inputPath = "input.txt";

	if (argc >= 4)
	{
		stopsFile = argv[1];
		tripsFile = argv[2];
		stopTimesFile = argv[3];
	}
	if (argc >= 5)
	{
		inputPath = argv[4];
	}

	std::ifstream inputFile(inputPath);
    if (!inputFile.is_open())
    {
        std::cout << "Could not open input file." << std::endl;
        return 0;
    }
	std::ofstream outputFile(outputPath);
	if (!outputFile.is_open())
	{
		std::cout << "Could not open output file." << std::endl;
		return 0;
	}
    auto network = NetworkLoader::load(stopsFile, tripsFile, stopTimesFile);

    GeneticAlgorithm genAlg;
    ACOAlgorithm acoAlg;
    PSOAlgorithm psoAlg;
    RouteFinder genAlgRouteFinder(network, genAlg);
    RouteFinder acoAlgRouteFinder(network, acoAlg);
    RouteFinder psoAlgRouteFinder(network, psoAlg);

    std::string line;

    while (getline(inputFile, line))
    {
        int startId = 0;
        int endId = 0;
        std::string timeStr = "00:00";

        std::vector<Result> genResults;
        std::vector<Result> acoResults;
        std::vector<Result> psoResults;

		std::stringstream ss(line);
		ss >> startId >> endId >> timeStr;
		const Stop* start = network.getStop(startId);
		const Stop* end = network.getStop(endId);
		if (!start)
		{
			std::cout << "Invalid start stop ID: " << startId << std::endl;
			continue;
		}
		if (!end)
		{
			std::cout << "Invalid end stop ID: " << endId << std::endl;
			continue;
		}
		std::chrono::minutes departureTime;
		try {
			departureTime = parseTime(timeStr);
		}
		catch (std::exception ex)
		{
            continue;
		}

        for (int i = 0; i < 10; i++)
        {
            auto fGen = std::async(std::launch::async, [&] {
                auto s = std::chrono::steady_clock::now();
                auto r = genAlgRouteFinder.findRoute(start, end, departureTime);
                auto e = std::chrono::steady_clock::now();
                auto d = std::chrono::duration_cast<std::chrono::milliseconds>(e - s);
                return std::make_pair(r, d);
                });

            auto fAco = std::async(std::launch::async, [&] {
                auto s = std::chrono::steady_clock::now();
                auto r = acoAlgRouteFinder.findRoute(start, end, departureTime);
                auto e = std::chrono::steady_clock::now();
                auto d = std::chrono::duration_cast<std::chrono::milliseconds>(e - s);
                return std::make_pair(r, d);
                });

            auto fPso = std::async(std::launch::async, [&] {
                auto s = std::chrono::steady_clock::now();
                auto r = psoAlgRouteFinder.findRoute(start, end, departureTime);
                auto e = std::chrono::steady_clock::now();
                auto d = std::chrono::duration_cast<std::chrono::milliseconds>(e - s);
                return std::make_pair(r, d);
                });

            auto [resGen, tGen] = fGen.get();
            auto [resAco, tAco] = fAco.get();
            auto [resPso, tPso] = fPso.get();

            genResults.emplace_back(resGen, tGen);
            acoResults.emplace_back(resAco, tAco);
            psoResults.emplace_back(resPso, tPso);
        }



    }
}
