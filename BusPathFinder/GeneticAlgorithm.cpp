#include "GeneticAlgorithm.h"

#include <random>
#include <algorithm>
#include <iostream>
#include <unordered_set>

#define POPULATION 30
#define GENERATIONS 100


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

static double randomDouble(double a, double b)
{
    std::uniform_real_distribution<double> dist(a, b);
    return dist(rng);
}

inline StopTime* getRandomDeparture(std::vector<StopTime*> departureOptions)
{
    return departureOptions[randomInt(0, departureOptions.size() - 1)];
}

struct Individual
{
    std::vector<ConnectionTime> genes;
    bool isValid;

    // funkcje celu
    std::chrono::minutes arrivalTime;
    double travelTime = 0.0;
    double cost = 0.0;
    int transfers = 0;

    // NSGA-II
    int rank = 0;
    double crowdingDistance = 0.0;

    // pomocnicze
    int dominationCount = 0;
    std::vector<int> dominated = {};
};

static bool dominates(const Individual& a, const Individual& b)
{
    if (a.arrivalTime > b.arrivalTime || a.travelTime > b.travelTime || a.cost > b.cost || a.transfers > b.transfers)
        return false;

    if (a.arrivalTime < b.arrivalTime || a.travelTime < b.travelTime || a.cost < b.cost || a.transfers < b.transfers)
        return true;

    return false;
}

static void evaluateIndividual(Individual& individual, const Stop* targetStop)
{
    individual.isValid =
        !individual.genes.empty() &&
        individual.genes.back().to->getStop() == targetStop;

    if (!individual.isValid)
    {
        individual.arrivalTime = std::chrono::minutes::max() / 2;
        individual.travelTime = 1e9;
        individual.cost = 1e9;
        individual.transfers = 1e9;
        return;
    }  

    auto startTime = individual.genes.front().from->getTime();
    auto endTime = individual.genes.back().to->getTime();

    individual.arrivalTime = endTime;

    individual.travelTime =
        std::chrono::duration_cast<std::chrono::minutes>(
            endTime - startTime
        ).count();

    individual.cost = 0.0;
    individual.transfers = 0;

    Trip* previousTrip = nullptr;

    for (const auto& connection : individual.genes)
    {
        auto trip = connection.from->getTrip();

        if (previousTrip && previousTrip != trip)
            individual.transfers++;

        // TODO:
        // dodaj wyliczanie kosztu
        // np. strefy + linia pospieszna

        previousTrip = trip;
    }
}

static std::vector<std::vector<int>> nonDominatedSort(std::vector<Individual>& population)
{
    std::vector<std::vector<int>> fronts;
    std::vector<int> firstFront;

    for (int i = 0; i < population.size(); i++)
    {
        population[i].dominated.clear();
        population[i].dominationCount = 0;

        for (int j = 0; j < population.size(); j++)
        {
            if (i == j)
                continue;

            if (dominates(population[i], population[j]))
            {
                population[i].dominated.push_back(j);
            }
            else if (dominates(population[j], population[i]))
            {
                population[i].dominationCount++;
            }
        }

        if (population[i].dominationCount == 0)
        {
            population[i].rank = 0;
            firstFront.push_back(i);
        }
    }

    fronts.push_back(firstFront);

    int frontIndex = 0;

    while (!fronts[frontIndex].empty())
    {
        std::vector<int> nextFront;

        for (int i : fronts[frontIndex])
        {
            for (int j : population[i].dominated)
            {
                population[j].dominationCount--;

                if (population[j].dominationCount == 0)
                {
                    population[j].rank = frontIndex + 1;
                    nextFront.push_back(j);
                }
            }
        }

        frontIndex++;
        fronts.push_back(nextFront);
    }

    return fronts;
}

void computeCrowdingDistance(
    std::vector<Individual>& population,
    const std::vector<int>& front)
{
    if (front.empty())
        return;

    for (int idx : front)
        population[idx].crowdingDistance = 0.0;

    // ===== PRZYJAZD =====
    auto sorted = front;

    std::sort(sorted.begin(), sorted.end(),
        [&](int a, int b)
        {
            return population[a].arrivalTime < population[b].arrivalTime;
        });

    population[sorted.front()].crowdingDistance = INFINITY;
    population[sorted.back()].crowdingDistance = INFINITY;

    double minVal = population[sorted.front()].arrivalTime.count() * 1.0;
    double maxVal = population[sorted.back()].arrivalTime.count() * 1.0;

    if (maxVal > minVal)
    {
        for (int i = 1; i < sorted.size() - 1; i++)
        {
            population[sorted[i]].crowdingDistance +=
                (population[sorted[i + 1]].arrivalTime.count() -
                    population[sorted[i - 1]].arrivalTime.count()) * 1.0
                / (maxVal - minVal);
        }
    }


    // ===== CZAS =====
    std::sort(sorted.begin(), sorted.end(),
        [&](int a, int b)
        {
            return population[a].travelTime < population[b].travelTime;
        });

    population[sorted.front()].crowdingDistance = INFINITY;
    population[sorted.back()].crowdingDistance = INFINITY;

    minVal = population[sorted.front()].travelTime;
    maxVal = population[sorted.back()].travelTime;

    if (maxVal > minVal)
    {
        for (int i = 1; i < sorted.size() - 1; i++)
        {
            population[sorted[i]].crowdingDistance +=
                (population[sorted[i + 1]].travelTime -
                    population[sorted[i - 1]].travelTime)
                / (maxVal - minVal);
        }
    }

    // ===== KOSZT =====
    std::sort(sorted.begin(), sorted.end(),
        [&](int a, int b)
        {
            return population[a].cost < population[b].cost;
        });

    population[sorted.front()].crowdingDistance = INFINITY;
    population[sorted.back()].crowdingDistance = INFINITY;

    minVal = population[sorted.front()].cost;
    maxVal = population[sorted.back()].cost;

    if (maxVal > minVal)
    {
        for (int i = 1; i < sorted.size() - 1; i++)
        {
            population[sorted[i]].crowdingDistance +=
                (population[sorted[i + 1]].cost -
                    population[sorted[i - 1]].cost)
                / (maxVal - minVal);
        }
    }

    // ===== PRZESIADKI =====
    std::sort(sorted.begin(), sorted.end(),
        [&](int a, int b)
        {
            return population[a].transfers < population[b].transfers;
        });

    population[sorted.front()].crowdingDistance = INFINITY;
    population[sorted.back()].crowdingDistance = INFINITY;

    minVal = population[sorted.front()].transfers;
    maxVal = population[sorted.back()].transfers;

    if (maxVal > minVal)
    {
        for (int i = 1; i < sorted.size() - 1; i++)
        {
            population[sorted[i]].crowdingDistance +=
                (population[sorted[i + 1]].transfers -
                    population[sorted[i - 1]].transfers)
                / (maxVal - minVal);
        }
    }
}

Individual tournamentSelection(const std::vector<Individual>& population)
{
    int a = randomInt(0, population.size() - 1);
    int b = randomInt(0, population.size() - 1);

    const auto& p1 = population[a];
    const auto& p2 = population[b];

    if (p1.rank < p2.rank)
        return p1;

    if (p2.rank < p1.rank)
        return p2;

    if (p1.crowdingDistance > p2.crowdingDistance)
        return p1;

    return p2;
}

Individual crossover(
    const Individual& parent1,
    const Individual& parent2)
{
    Individual child;

    for (int i = 0; i < parent1.genes.size(); i++)
    {
        auto stop = parent1.genes[i].to->getStop();
        auto time = parent1.genes[i].to->getTime();

        auto it = std::find_if(
            parent2.genes.begin(),
            parent2.genes.end(),
            [&](const auto& elem)
            {
                return elem.to->getStop() == stop && elem.to->getTime() >= time;
            });

        if (it != parent2.genes.end())
        {
            child.genes.insert(
                child.genes.end(),
                parent1.genes.begin(),
                parent1.genes.begin() + i + 1);

            child.genes.insert(
                child.genes.end(),
                it + 1,
                parent2.genes.end());

            return child;
        }
    }

    return parent1;
}

void mutate(
    Individual& individual,
    const Network& network,
    const Stop* end)
{
    if (individual.genes.size() < 2)
        return;

    int mutationPoint =
        randomInt(0, individual.genes.size() - 1);

    auto stopTime = individual.genes[mutationPoint].to;

    individual.genes.resize(mutationPoint + 1);

    while (individual.genes.back().to->getStop() != end)
    {
        auto departures = network.getStopTimes(
            stopTime->getStop(),
            stopTime->getTime());

        if (departures.empty())
            break;

        auto current = getRandomDeparture(departures);
        auto next = current->getNextStopTime();

        individual.genes.push_back(
            ConnectionTime{ current, next });

        stopTime = next;
    }
}

static std::string pathSignature(const Individual& individual)
{
    std::string sig;

    for (const auto& gene : individual.genes)
    {
        sig += std::to_string(
            gene.from->getTrip()->getService()->getId());

        sig += "|";

        sig += std::to_string(
            gene.from->getStop()->getId());

        sig += "|";

        sig += std::to_string(
            gene.from->getTime().count());

        sig += "->";
    }

    return sig;
}

std::vector<Path> GeneticAlgorithm::findPath(const Network& network, const Stop* start, const Stop* end, std::chrono::minutes departureTime)
{
    auto startOptions = network.getStopTimes(start, departureTime);

    if (startOptions.empty() || start == end)
        return { Path() };

    // populacja
    std::vector<Individual> population;

    for (int i = 0; i < POPULATION; i++)
    {
        Individual individual;

        std::unordered_set<const Stop*> visited;
        
        //wylosuj rozpoczecie podrozy
        auto currentStopTime = getRandomDeparture(startOptions);
        //weź następny przystanek z tej samej trasy
        auto nextStopTime = currentStopTime->getNextStopTime();
        individual.genes.push_back(ConnectionTime{ currentStopTime, nextStopTime });

        while (individual.genes.back().to->getStop() != end)
        {
			//wez opcje kontynuacji podrozy z tego przystanku
            auto departureOptions = network.getStopTimes(nextStopTime->getStop(), nextStopTime->getTime());

            visited.insert(currentStopTime->getStop());

            std::vector<StopTime*> filteredDepartureOptions;
            for(auto& el: departureOptions)
                if (visited.find(el->getNextStopTime()->getStop()) == visited.end())
                    filteredDepartureOptions.push_back(el);
            
			//jesli brak opcji, to przerwij
            if (filteredDepartureOptions.empty())
                break;

			//50% szans, że wybierzemy kontynuacje tej samej trasy
            auto it = std::find_if(filteredDepartureOptions.begin(), filteredDepartureOptions.end(),
                [&](const auto& elem)
                {
                    return elem->getTrip() == currentStopTime->getTrip();
                });

            if (it != filteredDepartureOptions.end() && randomInt(0, 100) < 85)
                currentStopTime = *it;
            else
			    currentStopTime = getRandomDeparture(filteredDepartureOptions);

            //weź następny przystanek z tej samej trasy
            nextStopTime = currentStopTime->getNextStopTime();
            individual.genes.push_back(ConnectionTime{ currentStopTime, nextStopTime });
        }
        
        population.push_back(individual);
    }

    for (int generation = 0; generation < GENERATIONS; generation++)
    {
        // ocena
        for (auto& individual : population)
            evaluateIndividual(individual, end);

        // sortowanie Pareto
        auto fronts = nonDominatedSort(population);

        for (const auto& front : fronts)
            computeCrowdingDistance(population, front);

        // potomstwo
        std::vector<Individual> offspring;

        std::unordered_set<std::string> offspringSignatures;

        while (offspring.size() < population.size())
        {
            auto parent1 = tournamentSelection(population);
            auto parent2 = tournamentSelection(population);

            auto child = crossover(parent1, parent2);

            if (randomDouble(0.0, 1.0) < 0.1)
                mutate(child, network, end);

            evaluateIndividual(child, end);

            auto sig = pathSignature(child);

            if (offspringSignatures.find(sig) != offspringSignatures.end())
                continue;

            offspringSignatures.insert(sig);

            offspring.push_back(child);
        }

        // połączenie populacji
        std::vector<Individual> combined = population;

        combined.insert(
            combined.end(),
            offspring.begin(),
            offspring.end());

        // ponowne sortowanie
        auto combinedFronts = nonDominatedSort(combined);

        population.clear();

        std::unordered_set<std::string> populationSignatures;

        for (const auto& front : combinedFronts)
        {
            computeCrowdingDistance(combined, front);

            if (population.size() + front.size() <= POPULATION)
            {
                for (int idx : front)
                {
                    auto sig = pathSignature(combined[idx]);

                    if (populationSignatures.find(sig) != populationSignatures.end())
                        continue;

                    populationSignatures.insert(sig);
                    population.push_back(combined[idx]);
                }
            }
            else
            {
                auto sortedFront = front;

                std::sort(sortedFront.begin(), sortedFront.end(),
                    [&](int a, int b)
                    {
                        return combined[a].crowdingDistance >
                            combined[b].crowdingDistance;
                    });

                for (int idx : sortedFront)
                {
                    if (population.size() >= POPULATION)
                        break;

                    auto sig = pathSignature(combined[idx]);

                    if (populationSignatures.find(sig) != populationSignatures.end())
                        continue;

                    populationSignatures.insert(sig);
                    population.push_back(combined[idx]);
                }

                break;
            }
        }
    }

    auto fronts = nonDominatedSort(population);

    std::vector<Path> results;

    for (int idx : fronts[0])
    {
        Path path(population[idx].genes);

        results.push_back(path);
    }

    return results;
}
