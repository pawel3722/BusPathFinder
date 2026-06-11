#pragma once
#include "IAlgorithm.h"

struct Particle
{
    std::vector<ConnectionTime> path;
    std::vector<ConnectionTime> personalBest;

    bool isValid = false;

    Objective objective;

    std::chrono::minutes arrivalTime = std::chrono::minutes::max();
    std::chrono::minutes travelTime = std::chrono::minutes::max();
    std::chrono::minutes waitingTime = std::chrono::minutes(0);
    int transfers = 0;

    double distanceToGoal = 1e9;
};

class PSOAlgorithm : public IAlgorithm
{
	int SWARM_SIZE = 100;
	int ITERATIONS = 100;
	int MAX_PATH_LENGTH = 70;

	int MIN_TRANSFER_TIME = 3;
	int MAX_DEPARTURES_PER_ROUTE = 3;
	double SAME_TRIP_PROB = 0.95;

	double RANDOM_MUTATION_PROB = 0.25;
	double PBEST_PROB = 0.45;
	double GBEST_PROB = 0.45;
public:
    PSOAlgorithm() {}
    PSOAlgorithm(const std::string& configFilePath);
	std::vector<Path> findPath(const Network& network, const Stop* start, const Stop* end, std::chrono::minutes departureTime);
private:
    bool dominates(const Particle& a, const Particle& b);
    std::vector<Trip*> buildTripSequence(const std::vector<ConnectionTime>& path);
    double pathSimilarity(const Particle& a, const Particle& b);
    double heuristicValue(Objective objective, StopTime* dep, const Stop* end, std::chrono::minutes arrival, StopTime* previous);
    bool containsEdge(const std::vector<ConnectionTime>& path, const ConnectionTime& edge);
    StopTime* chooseNextDeparture(const std::vector<StopTime*>& departureOptions, const Stop* end, std::chrono::minutes arrival, StopTime* previous, 
        Objective objective, const std::vector<ConnectionTime>* pbest = nullptr, const std::vector<ConnectionTime>* gbest = nullptr);
    Particle buildParticle(const Network& network, const Stop* start, const Stop* end, std::chrono::minutes departureTime);
    void evaluateParticle(Particle& particle, std::chrono::minutes departureTime, const Stop* end);
    void updateArchive(std::vector<Particle>& archive, const Particle& candidate);
    const Particle& selectLeader(const std::vector<Particle>& archive, Objective objective);
    void rerouteFromIndex(Particle& particle, const Particle& leader, const Network& network, const Stop* end, int splitIndex);
    void followPath(Particle& particle, const Particle& leader, const std::vector<ConnectionTime>& target, const Network& network, const Stop* end);
};