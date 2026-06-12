#include <iostream>
#include <fstream>
#include <sstream>
#define NOMINMAX
#include <windows.h>
#include <string>
#include <future>
#include "NetworkLoaderGdansk.h"
#include "NetworkLoaderGZM.h"
#include "NetworkLoaderZG.h"
#include "GeneticAlgorithm.h"
#include "RouteFinder.h"
#include "Functions.h" 
#include "ACOAlgorithm.h"
#include "PSOAlgorithm.h"
#include "Result.h"

std::unordered_map<std::string, std::string> readConfigFile()
{
    std::cout << "Podaj sciezke do pliku konfiguracyjnego:" << std::endl;
    std::string configFilePath;
    std::cin >> configFilePath;

    if (configFilePath == "")
        configFilePath = "config.txt";

    std::ifstream file(configFilePath);

    if (!file)
        throw std::runtime_error("Cannot open config file: " + configFilePath);

    std::unordered_map<std::string, std::string> params;

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        auto pos = line.find('=');
        if (pos == std::string::npos)
            continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        params[key] = value;
    }

    return params;
}

void test()
{
    auto config = readConfigFile();
    int gtfsType = std::stoi(config.at("GTFS_TYPE"));

    GeneticAlgorithm genAlg(config.at("CONFIG_GEN"));
    ACOAlgorithm acoAlg(config.at("CONFIG_ACO"));
    PSOAlgorithm psoAlg(config.at("CONFIG_PSO"));

    Network network =
        (gtfsType == 0) ? NetworkLoaderZG::load(config.at("DATA_PATH")) :
        (gtfsType == 1) ? NetworkLoaderGdansk::load(config.at("DATA_PATH")) :
        NetworkLoaderGZM::load(config.at("DATA_PATH"));

    RouteFinder genAlgRouteFinder(network, genAlg);
    RouteFinder acoAlgRouteFinder(network, acoAlg);
    RouteFinder psoAlgRouteFinder(network, psoAlg);

    int startId = 0;
    int endId = 0;
    std::string timeStr = "00:00";

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
            return;
        }

        if (!end)
        {
            std::cout << "Invalid end stop ID\n";
            startId = 0;
            return;
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

    int validCount = 0;

    for (const auto& el : vec)
    {
        if (!el.isValid)
            continue;
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
        validCount++;
    }

    if (!validCount)
    {
        os << "No valid result found!" << std::endl;
        return;
    }

    avgArrivalTime /= validCount;
    avgTravelTime /= validCount;
    avgWaitingTime /= validCount;
    avgComputationTime /= validCount;
    avgTransfers /= validCount;
    avgParetoSize /= validCount;


    for (const auto& el : vec)
    {
        if (!el.isValid)
            continue;

        sqSumArrivalTime += std::pow(el.bestArrivalTime.count() - avgArrivalTime, 2);
        sqSumTravelTime += std::pow(el.bestTravelTime.count() - avgTravelTime, 2);
        sqSumWaitingTime += std::pow(el.bestWaitingTime.count() - avgWaitingTime, 2);
        sqSumComputationTime += std::pow(el.computationTime.count() - avgComputationTime, 2);
        sqSumTransfers += std::pow(el.bestTransfers - avgTransfers, 2);
        sqSumParetoSize += std::pow(el.paths.size() - avgParetoSize, 2);
    }

    double stdDevArrivalTime = std::sqrt(sqSumArrivalTime / validCount);
    double stdDevTravelTime = std::sqrt(sqSumTravelTime / validCount);
    double stdDevWaitingTime = std::sqrt(sqSumWaitingTime / validCount);
    double stdDevComputationTime = std::sqrt(sqSumComputationTime / validCount);
    double stdDevTransfers = std::sqrt(sqSumTransfers / validCount);
    double stdDevParetoSize = std::sqrt(sqSumParetoSize / validCount);

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


void benchmark()
{
    auto config = readConfigFile();
    int gtfsType = std::stoi(config.at("GTFS_TYPE"));

    Network network =
        (gtfsType == 0) ? NetworkLoaderZG::load(config.at("DATA_PATH")) :
        (gtfsType == 1) ? NetworkLoaderGdansk::load(config.at("DATA_PATH")) :
        NetworkLoaderGZM::load(config.at("DATA_PATH"));

    GeneticAlgorithm genAlg(config.at("CONFIG_GEN"));
    ACOAlgorithm acoAlg(config.at("CONFIG_ACO"));
    PSOAlgorithm psoAlg(config.at("CONFIG_PSO"));


    std::string outputPath = config.at("OUTPUT_PATH");
    std::string outputCsvPath = config.at("OUTPUT_CSV_PATH");
	std::string inputPath = config.at("INPUT_PATH");

	std::ifstream inputFile(inputPath);
    if (!inputFile.is_open())
    {
        std::cout << "Could not open input file." << std::endl;
        return;
    }
	std::ofstream outputFile(outputPath);
	if (!outputFile.is_open())
	{
		std::cout << "Could not open output file." << std::endl;
		return;
	}
    std::ofstream outputCsvFile(outputCsvPath);
    if (!outputCsvFile.is_open())
    {
        std::cout << "Could not open output csv file." << std::endl;
        return;
    }

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
                if (el.isValid())
                    outputCsvFile << experiment << ",GEN," << i << ",1," << el.getArrivalTime().count() << ',' << el.getTravelTime().count() << ',' << el.getWaitingTime().count() << ',' << el.getTransfers() << ',' << tGen.count() << '\n';
                else
                    outputCsvFile << experiment << ",GEN," << i << ",0,0,0,0,0," << tGen.count() << '\n';
            }
            for (const auto& el : resAco)
            {
                if (el.isValid())
                    outputCsvFile << experiment << ",ACO," << i << ",1," << el.getArrivalTime().count() << ',' << el.getTravelTime().count() << ',' << el.getWaitingTime().count() << ',' << el.getTransfers() << ',' << tAco.count() << '\n';
                else
                    outputCsvFile << experiment << ",ACO," << i << ",0,0,0,0,0," << tAco.count() << '\n';
            }
            for (const auto& el : resPso)
            {
                if (el.isValid())
                    outputCsvFile << experiment << ",PSO," << i << ",1," << el.getArrivalTime().count() << ',' << el.getTravelTime().count() << ',' << el.getWaitingTime().count() << ',' << el.getTransfers() << ',' << tPso.count() << '\n';
                else
                    outputCsvFile << experiment << ",PSO," << i << ",0,0,0,0,0," << tPso.count() << '\n';
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

int main()
{
    while (true)
    {
#ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
#endif

        std::cout << "Wybierz opcje dzialania programu:" << std::endl
            << "Test - 1" << std::endl
            << "Badania - 2" << std::endl
            << "Wyjście - 0" << std::endl;
        std::string input;
        std::cin >> input;
        if (input == "1")
            test();
        else if (input == "2")
            benchmark();
        else if (input == "0")
            break;
        else
            std::cout << "Niepoprawny numer!" << std::endl;
    }
}
