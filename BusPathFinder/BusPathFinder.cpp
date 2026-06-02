#include <iostream>
#include <fstream>
#include <sstream>
#define NOMINMAX
#include <windows.h>
#include <string>
#include <future>
#include "NetworkLoaderGdansk.h"
#include "NetworkLoaderGZM.h"
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



   auto network = NetworkLoaderGdansk::load(".\\Gdansk", "20260602");
   //auto network = NetworkLoaderGZM::load(".\\GZM");

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

void printOutput(std::ofstream& os, std::vector<Result>& vec, std::string header)
{
    os << header << std::endl;

    if (vec.empty())
    {
        os << "No results" << std::endl;
        return;
    }

    std::chrono::minutes bestArrivalTime = std::chrono::minutes::max();
    std::chrono::minutes bestTravelTime = std::chrono::minutes::max();
    std::chrono::minutes bestWaitingTime = std::chrono::minutes::max();
    std::chrono::milliseconds bestComputationTime = std::chrono::milliseconds::max();
    int bestTransfers = INT_MAX;
    int bestParetoSize = 0;

    std::chrono::minutes worstArrivalTime = std::chrono::minutes::zero();
    std::chrono::minutes worstTravelTime = std::chrono::minutes::zero();
    std::chrono::minutes worstWaitingTime = std::chrono::minutes::zero();
    std::chrono::milliseconds worstComputationTime = std::chrono::milliseconds::zero();
    int worstTransfers = 0;
    int worstParetoSize = INT_MAX;

    double avgArrivalTime = 0;
    double avgTravelTime = 0;
    double avgWaitingTime = 0;
    double avgComputationTime = 0;
    double avgTransfers = 0;
    double avgParetoSize = 0;

    double sqSumArrivalTime = 0;
    double sqSumTravelTime = 0;
    double sqSumWaitingTime = 0;
    double sqSumComputationTime = 0;
    double sqSumTransfers = 0;
    double sqSumParetoSize = 0;

    for (const auto& el : vec)
    {
        if (el.bestArrivalTime < bestArrivalTime)
            bestArrivalTime = el.bestArrivalTime;
        if (el.bestArrivalTime > worstArrivalTime)
            worstArrivalTime = el.bestArrivalTime;
        avgArrivalTime += el.bestArrivalTime.count();

        if (el.bestTravelTime < bestTravelTime)
            bestTravelTime = el.bestTravelTime;
        if (el.bestTravelTime > worstTravelTime)
            worstTravelTime = el.bestTravelTime;
        avgTravelTime += el.bestTravelTime.count();

        if (el.bestWaitingTime < bestWaitingTime)
            bestWaitingTime = el.bestWaitingTime;
        if (el.bestWaitingTime > worstWaitingTime)
            worstWaitingTime = el.bestWaitingTime;
        avgWaitingTime += el.bestWaitingTime.count();

        if (el.computationTime < bestComputationTime)
            bestComputationTime = el.computationTime;
        if (el.computationTime > worstComputationTime)
            worstComputationTime = el.computationTime;
        avgComputationTime += el.computationTime.count();

        if (el.bestTransfers < bestTransfers)
            bestTransfers = el.bestTransfers;
        if (el.bestTransfers > worstTransfers)
            worstTransfers = el.bestTransfers;
        avgTransfers += el.bestTransfers;

        if (el.paths.size() > bestParetoSize)
            bestParetoSize = el.paths.size();
        if (el.paths.size() < worstParetoSize)
            worstParetoSize = el.paths.size();
        avgParetoSize += el.paths.size();
    }

    avgArrivalTime /= vec.size();
    avgTravelTime /= vec.size();
    avgWaitingTime /= vec.size();
    avgComputationTime /= vec.size();
    avgTransfers /= vec.size();
    avgParetoSize /= vec.size();


    for (const auto& el : vec)
    {
        sqSumArrivalTime += std::pow(el.bestArrivalTime.count() - avgArrivalTime, 2);
        sqSumTravelTime += std::pow(el.bestTravelTime.count() - avgTravelTime, 2);
        sqSumWaitingTime += std::pow(el.bestWaitingTime.count() - avgWaitingTime, 2);
        sqSumComputationTime += std::pow(el.computationTime.count() - avgComputationTime, 2);
        sqSumTransfers += std::pow(el.bestTransfers - avgTransfers, 2);
        sqSumParetoSize += std::pow(el.paths.size() - avgParetoSize, 2);
    }

    double stdDevArrivalTime = std::sqrt(sqSumArrivalTime / vec.size());
    double stdDevTravelTime = std::sqrt(sqSumTravelTime / vec.size());
    double stdDevWaitingTime = std::sqrt(sqSumWaitingTime / vec.size());
    double stdDevComputationTime = std::sqrt(sqSumComputationTime / vec.size());
    double stdDevTransfers = std::sqrt(sqSumTransfers / vec.size());
    double stdDevParetoSize = std::sqrt(sqSumParetoSize / vec.size());

    os << std::left
        << std::fixed
        << std::setprecision(2);

    auto fmt = [](double v)
        {
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(2) << v;
            return ss.str();
        };

    os << std::setw(20) << "Arrival time:"
        << std::setw(18) << ("min: " + formatTime(bestArrivalTime))
        << std::setw(18) << ("max: " + formatTime(worstArrivalTime))
        << std::setw(18) << ("avg: " + formatTime(std::chrono::minutes(
            static_cast<long long>(std::round(avgArrivalTime)))))
        << std::setw(22) << ("std dev: " + formatTime(std::chrono::minutes(
            static_cast<long long>(std::round(stdDevArrivalTime)))))
        << '\n';

    os << std::setw(20) << "Travel time:"
        << std::setw(18) << ("min: " + formatTime(bestTravelTime))
        << std::setw(18) << ("max: " + formatTime(worstTravelTime))
        << std::setw(18) << ("avg: " + formatTime(std::chrono::minutes(
            static_cast<long long>(std::round(avgTravelTime)))))
        << std::setw(22) << ("std dev: " + formatTime(std::chrono::minutes(
            static_cast<long long>(std::round(stdDevTravelTime)))))
        << '\n';

    os << std::setw(20) << "Waiting time:"
        << std::setw(18) << ("min: " + formatTime(bestWaitingTime))
        << std::setw(18) << ("max: " + formatTime(worstWaitingTime))
        << std::setw(18) << ("avg: " + formatTime(std::chrono::minutes(
            static_cast<long long>(std::round(avgWaitingTime)))))
        << std::setw(22) << ("std dev: " + formatTime(std::chrono::minutes(
            static_cast<long long>(std::round(stdDevWaitingTime)))))
        << '\n';

    os << std::setw(20) << "Transfers:"
        << std::setw(18) << ("min: " + std::to_string(bestTransfers))
        << std::setw(18) << ("max: " + std::to_string(worstTransfers))
        << std::setw(18) << ("avg: " + fmt(avgTransfers))
        << std::setw(22) << ("std dev: " + fmt(stdDevTransfers))
        << '\n';

    os << std::setw(20) << "Pareto size:"
        << std::setw(18) << ("min: " + std::to_string(worstParetoSize))
        << std::setw(18) << ("max: " + std::to_string(bestParetoSize))
        << std::setw(18) << ("avg: " + fmt(avgParetoSize))
        << std::setw(22) << ("std dev: " + fmt(stdDevParetoSize))
        << '\n';

    os << std::setw(20) << "Computation time:"
        << std::setw(18) << ("min: " + std::to_string(bestComputationTime.count()) + "ms")
        << std::setw(18) << ("max: " + std::to_string(worstComputationTime.count()) + "ms")
        << std::setw(18) << ("avg: " + fmt(avgComputationTime) + "ms")
        << std::setw(22) << ("std dev: " + fmt(stdDevComputationTime) + "ms")
        << '\n';
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
	std::string outputCsvPath = "output.csv";
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
    std::ofstream outputCsvFile(outputCsvPath);
    if (!outputCsvFile.is_open())
    {
        std::cout << "Could not open output csv file." << std::endl;
        return 0;
    }

    auto network = NetworkLoaderGdansk::load(".\\Gdansk", "20260602");

    GeneticAlgorithm genAlg;
    ACOAlgorithm acoAlg;
    PSOAlgorithm psoAlg;
    RouteFinder genAlgRouteFinder(network, genAlg);
    RouteFinder acoAlgRouteFinder(network, acoAlg);
    RouteFinder psoAlgRouteFinder(network, psoAlg);

    std::string line;
    int experiment = 0;

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

        std::cout << ">>>>>Experiment: " << experiment << " TRIP FROM: " << start->getName() << " <" << start->getId() << "> TO: " << end->getName() << " <" << end->getId() << "> AT: " << formatTime(departureTime) << " <<<<<" << std::endl;
        outputFile << ">>>>>>>>>>Experiment: " << experiment << " TRIP FROM: " << start->getName() << " <" << start->getId() << "> TO: " << end->getName() << " <" << end->getId() << "> AT: " << formatTime(departureTime) << " <<<<<<<<<<" << std::endl;

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

            std::cout << "Iteration " << i+1 << "/10" << std::endl;

            for (const auto& el : resGen)
            {
                outputCsvFile << experiment << ",GEN," << i << ',' << el.getArrivalTime().count() << ',' << el.getTravelTime().count() << ',' << el.getWaitingTime().count() << ',' << el.getTransfers() << ',' << tGen.count() << '\n';
            }
            for (const auto& el : resAco)
            {
                outputCsvFile << experiment << ",ACO," << i << ',' << el.getArrivalTime().count() << ',' << el.getTravelTime().count() << ',' << el.getWaitingTime().count() << ',' << el.getTransfers() << ',' << tAco.count() << '\n';
            }
            for (const auto& el : resPso)
            {
                outputCsvFile << experiment << ",PSO," << i << ',' << el.getArrivalTime().count() << ',' << el.getTravelTime().count() << ',' << el.getWaitingTime().count() << ',' << el.getTransfers() << ',' << tPso.count() << '\n';
            }

            genResults.emplace_back(resGen, tGen);
            acoResults.emplace_back(resAco, tAco);
            psoResults.emplace_back(resPso, tPso);
        }
        printOutput(outputFile, genResults, "++++++++++++++++++++++++++++++++++++GEN++++++++++++++++++++++++++++++++++++");
        printOutput(outputFile, acoResults, "++++++++++++++++++++++++++++++++++++ACO++++++++++++++++++++++++++++++++++++");
        printOutput(outputFile, psoResults, "++++++++++++++++++++++++++++++++++++PSO++++++++++++++++++++++++++++++++++++");

        experiment++;
    }
}
