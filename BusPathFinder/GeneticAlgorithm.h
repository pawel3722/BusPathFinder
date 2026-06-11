#pragma once
#include "IAlgorithm.h"

struct Individual
{
    std::vector<ConnectionTime> genes;
    bool isValid = false;

    // funkcje celu
    std::chrono::minutes arrivalTime = std::chrono::minutes(0);
    std::chrono::minutes travelTime = std::chrono::minutes(0);
    std::chrono::minutes waitingTime = std::chrono::minutes(0);
    int transfers = 0;

    // NSGA-II
    int rank = 0;
    double crowdingDistance = 0.0;

    // pomocnicze
    int dominationCount = 0;
    std::vector<int> dominated = {};
};

class GeneticAlgorithm : public IAlgorithm
{
	int POPULATION = 100;
	int GENERATIONS = 100;
	int MAX_PATH_LENGTH = 70;

	int MIN_TRANSFER_TIME = 3;
	int MAX_DEPARTURES_PER_ROUTE = 2;
	double SAME_TRIP_PROB = 0.95;

	double RANDOM_MUTATION_PROB = 0.3;
	double WAITING_MUTATION_PROB = 0.6;
	double TRANSFER_MUTATION_PROB = 0.4;

	public:
        GeneticAlgorithm() {}
		GeneticAlgorithm(const std::string& configFilePath);
		std::vector<Path> findPath(const Network& network, const Stop* start, const Stop* end, std::chrono::minutes departureTime) override;

	private:
		StopTime* chooseNextDeparture(std::vector<StopTime*>, const Stop*, std::chrono::minutes);
        double heuristicToGoal(const Individual&, const Stop*);
        bool dominates(const Individual&, const Individual&);
        void evaluateIndividual(Individual&, std::chrono::minutes, const Stop*);
        std::vector<std::vector<int>> nonDominatedSort(std::vector<Individual>&);
        void computeCrowdingDistance(std::vector<Individual>&, const std::vector<int>&);
        const Individual& tournamentSelection(const std::vector<Individual>&, const Stop*);
        Individual crossover(const Individual&, const Individual&, const Stop*);
        bool fixWaitingTimes(Individual&, const Network&);
        bool skipConnection(Individual&, const Network&);
        void mutate(Individual&, const Network&, const Stop*);
        std::string pathSignature(const Individual&);
};