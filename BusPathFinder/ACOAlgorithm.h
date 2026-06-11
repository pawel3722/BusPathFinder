#pragma once
#include "IAlgorithm.h"
#include <vector>
#include <chrono>

struct Ant
{
    std::vector<ConnectionTime> path;

    bool isValid = false;

    Objective objective;

    std::chrono::minutes arrivalTime = std::chrono::minutes::max();
    std::chrono::minutes travelTime = std::chrono::minutes::max();
    std::chrono::minutes waitingTime = std::chrono::minutes(0);
    int transfers = 0;

    double distanceToGoal = 1e9;
};

class ACOAlgorithm : public IAlgorithm
{
	int ANT_COUNT = 100;
	int ITERATIONS = 100;
	int MAX_PATH_LENGTH = 70;

	int MIN_TRANSFER_TIME = 3;
	int MAX_DEPARTURES_PER_ROUTE = 5;
    double SAME_TRIP_PROB = 0.95;

    double ALPHA = 0.7;
    double BETA = 0.3;
    double EVAPORATION = 0.1;
	double EPSILON = 0.1;

public:
    ACOAlgorithm() {}
    ACOAlgorithm(const std::string& configFilePath);
	std::vector<Path> findPath(const Network& network, const Stop* start, const Stop* end, std::chrono::minutes departureTime);

private:
    bool dominates(const Ant& a, const Ant& b);
    double calculateCost(const ConnectionTime& connection);
    std::vector<Trip*> buildTripSequence(const Ant& ant);
    double pathSimilarity(const Ant& a, const Ant& b);
    double getPheromone(const std::array<std::unordered_map<ConnectionTime, double, ConnectionTimeHash>, 5>& pheromones, Objective objective, const ConnectionTime& edge);
    double heuristicValue(Objective objective, StopTime* dep, const Stop* end, std::chrono::minutes arrival, StopTime* previous);
    StopTime* chooseNextDeparture(const std::vector<StopTime*>& departureOptions, const Stop* end, std::chrono::minutes arrival, 
        StopTime* previous, Objective objective, const std::array<std::unordered_map<ConnectionTime, double, ConnectionTimeHash>, 5>& pheromones);
    Ant buildAnt(const Network& network, const Stop* start, const Stop* end, std::chrono::minutes departureTime, const std::array<std::unordered_map<ConnectionTime, double, ConnectionTimeHash>, 5>& pheromones);
    void evaluateAnt(Ant& ant, std::chrono::minutes departureTime, const Stop* end);
    void evaporate(std::array<std::unordered_map<ConnectionTime, double, ConnectionTimeHash>, 5>& pheromones);
    void updateParetoArchive(std::vector<Ant>& archive, const Ant& candidate);
    void reinforce(const std::vector<Ant>& archive, std::array<std::unordered_map<ConnectionTime, double, ConnectionTimeHash>, 5>& pheromones, std::chrono::minutes departureReference);
    std::vector<Ant> selectBestRoutes(const std::vector<Ant>& archive);
};