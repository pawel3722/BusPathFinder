#include <random>
#include <algorithm>
#include <iostream>

#include "GeneticAlgorithm.h"
#include "Functions.h"


#define POPULATION 100
#define GENERATIONS 150
#define MAX_PATH_LENGTH 50
#define MUTATION_RATE 0.3

struct Individual
{
    std::vector<ConnectionTime> genes;
    bool isValid = false;

    // funkcje celu
    std::chrono::minutes arrivalTime = std::chrono::minutes(0);
    std::chrono::minutes travelTime = std::chrono::minutes(0);
    std::chrono::minutes waitingTime = std::chrono::minutes(0);
    double cost = 0.0;
    int transfers = 0;

    // NSGA-II
    int rank = 0;
    double crowdingDistance = 0.0;

    // pomocnicze
    int dominationCount = 0;
    std::vector<int> dominated = {};
};


double geoDistance(const Stop* a, const Stop* b)
{
    return haversine(a->getLat(), a->getLon(), b->getLat(), b->getLon());
}

static StopTime* chooseNextDeparture(std::vector<StopTime*> departureOptions, const Stop* end, std::chrono::minutes arrival, const Network& network)
{
    std::vector<double> weights;
    double weightSum = 0.0;

    for (auto* dep : departureOptions)
    {

        double currentDist = geoDistance(dep->getStop(), end);
        double nextDist = geoDistance(dep->getNextStopTime()->getStop(), end);
        double dist = nextDist - currentDist;

        double wait = (dep->getTime().count() - arrival.count()) * 1.0;

        double connectivity =network.getStopTimes (dep->getNextStopTime()->getStop(), dep->getNextStopTime()->getTime()).size();

        double weight =
            0.2 / (nextDist + 1.0)
            + 0.5 * connectivity
            + 0.3 / (wait + 1.0);

        weights.push_back(weight);
        weightSum += weight;
    }

    // cumulative distribution
    for (auto& w : weights)
        w /= weightSum;
    
    for (int i = 1; i < weights.size(); i++)
        weights[i] += weights[i - 1];

    double r = randomDouble(0.0, 1.0);

    for (int i = 0; i < weights.size(); i++)
    {
        if (r <= weights[i])
            return departureOptions[i];
    }

    return departureOptions.back();
}

static double heuristicToGoal(const Individual& ind, const Stop* end)
{
    if (ind.genes.empty())
        return 1e9;

    const Stop* current =
        ind.genes.back().to->getStop();

    return geoDistance(current, end);
}

static bool dominates(const Individual& a, const Individual& b)
{
    if (a.arrivalTime > b.arrivalTime || a.travelTime > b.travelTime || a.waitingTime > b.waitingTime || a.cost > b.cost || a.transfers > b.transfers)
        return false;

    if (a.arrivalTime < b.arrivalTime || a.travelTime < b.travelTime || a.waitingTime < b.waitingTime || a.cost < b.cost || a.transfers < b.transfers)
        return true;

    return false;
}

static void evaluateIndividual(Individual& individual, std::chrono::minutes departureTime, const Stop* targetStop)
{
    individual.isValid =
        !individual.genes.empty() &&
        individual.genes.back().to->getStop() == targetStop;

    if (!individual.isValid)
    {
        individual.arrivalTime = std::chrono::minutes::max() / 2;
        individual.travelTime = std::chrono::minutes::max() / 2;
        individual.waitingTime = std::chrono::minutes::max() / 2;
        individual.cost = 1e9;
        individual.transfers = 1e9;
        return;
    }

    auto startTime = individual.genes.front().from->getTime();
    auto endTime = individual.genes.back().to->getTime();

    individual.arrivalTime = endTime;

    individual.travelTime = std::chrono::duration_cast<std::chrono::minutes>(endTime - startTime);
    individual.waitingTime = std::chrono::minutes(0);

    individual.cost = 0.0;
    individual.transfers = 0;

    Trip* previousTrip = nullptr;

    
    auto arrival = departureTime;

    for (const auto& connection : individual.genes)
    {
        auto trip = connection.from->getTrip();

        if (previousTrip && previousTrip != trip)
        {
            individual.transfers++;
            individual.waitingTime += (connection.from->getTime() - arrival);
        }

        // TODO:
        // dodaj wyliczanie kosztu
        // np. strefy + linia pospieszna

        previousTrip = trip;
        arrival = connection.to->getTime();
    }
}

static std::vector<std::vector<int>> nonDominatedSort(
    std::vector<Individual>& population)
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

    while (frontIndex < fronts.size() &&
        !fronts[frontIndex].empty())
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

        if (!nextFront.empty())
            fronts.push_back(nextFront);

        frontIndex++;
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

    population[sorted.front()].crowdingDistance = 1e9;
    population[sorted.back()].crowdingDistance = 1e9;

    double minVal = population[sorted.front()].arrivalTime.count() * 1.0;
    double maxVal = population[sorted.back()].arrivalTime.count() * 1.0;

    if (maxVal > minVal)
    {
        for (int i = 1; i < sorted.size() - 1; i++)
        {
            population[sorted[i]].crowdingDistance +=
               ( population[sorted[i + 1]].arrivalTime.count() 
               - population[sorted[i - 1]].arrivalTime.count()) * 1.0
               / (maxVal - minVal);
        }
    }

    // ===== CZAS JAZDY =====
    std::sort(sorted.begin(), sorted.end(),
        [&](int a, int b)
        {
            return population[a].travelTime < population[b].travelTime;
        });

    // ===== CZAS JAZDY =====
    std::sort(sorted.begin(), sorted.end(),
        [&](int a, int b)
        {
            return population[a].travelTime < population[b].travelTime;
        });

    population[sorted.front()].crowdingDistance = INFINITY;
    population[sorted.back()].crowdingDistance = INFINITY;

    minVal = population[sorted.front()].travelTime.count() * 1.0;
    maxVal = population[sorted.back()].travelTime.count() * 1.0;

    if (maxVal > minVal)
    {
        for (int i = 1; i < sorted.size() - 1; i++)
        {
            population[sorted[i]].crowdingDistance +=
               ( population[sorted[i + 1]].travelTime.count() 
               - population[sorted[i - 1]].travelTime.count()) * 1.0
               / (maxVal - minVal);
        }
    }

    // ===== CZAS OCZEKIWANIA =====
    std::sort(sorted.begin(), sorted.end(),
        [&](int a, int b)
        {
            return population[a].waitingTime < population[b].waitingTime;
        });

    population[sorted.front()].crowdingDistance = 1e9;
    population[sorted.back()].crowdingDistance = 1e9;

    minVal = population[sorted.front()].waitingTime.count() * 1.0;
    maxVal = population[sorted.back()].waitingTime.count() * 1.0;

    if (maxVal > minVal)
    {
        for (int i = 1; i < sorted.size() - 1; i++)
        {
            population[sorted[i]].crowdingDistance +=
                (population[sorted[i + 1]].waitingTime.count()
                    - population[sorted[i - 1]].waitingTime.count()) * 1.0
                / (maxVal - minVal);
        }
    }

    // ===== KOSZT =====
    std::sort(sorted.begin(), sorted.end(),
        [&](int a, int b)
        {
            return population[a].cost < population[b].cost;
        });

    population[sorted.front()].crowdingDistance = 1e9;
    population[sorted.back()].crowdingDistance = 1e9;

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

    population[sorted.front()].crowdingDistance = 1e9;
    population[sorted.back()].crowdingDistance = 1e9;

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

Individual tournamentSelection(
    const std::vector<Individual>& population,
    const Stop* end)
{
    int a = randomInt(0, population.size() - 1);
    int b = randomInt(0, population.size() - 1);

    const auto& p1 = population[a];
    const auto& p2 = population[b];

    // rank
    if (p1.rank < p2.rank)
    {
        if (randomDouble(0.0, 1.0) < 0.8)
            return p1;

        return p2;
    }

    if (p2.rank < p1.rank)
    {
        if (randomDouble(0.0, 1.0) < 0.8)
            return p2;

        return p1;
    }

    // crowding
    if (p1.crowdingDistance > p2.crowdingDistance)
        return p1;

    if (p2.crowdingDistance > p1.crowdingDistance)
        return p2;

    // GPS heuristic
    double h1 = heuristicToGoal(p1, end);
    double h2 = heuristicToGoal(p2, end);

    if (h1 < h2)
        return p1;

    return p2;
}

Individual crossover(const Individual& parent1, const Individual& parent2, const Stop* end)
{
    Individual child;

    struct Candidate
    {
        int i;
        int j;
        double weight;
    };

    std::vector<Candidate> candidates;

    // znajdź wspólne punkty tras
    for (int i = 0; i < parent1.genes.size(); i++)
    {
        auto* stop1 = parent1.genes[i].to->getStop();
        auto time1 = parent1.genes[i].to->getTime();

        for (int j = 0; j < parent2.genes.size(); j++)
        {
            auto* stop2 = parent2.genes[j].to->getStop();
            auto time2 = parent2.genes[j].to->getTime();

            if (stop1 != stop2 || time2 < time1)
                continue;

            auto diff = abs((time2 - time1).count());

            // zbyt duża różnica czasu
            if (diff > 30)
                continue;

            double dist = geoDistance(stop1, end);

            candidates.push_back({
                i,
                j,
                1.0 / (dist + 1.0)
                });
        }
    }

    if (candidates.empty())
        return randomInt(0, 1) ? parent1 : parent2;

    double sum = 0.0;

    for (const auto& c : candidates)
        sum += c.weight;

    double r = randomDouble(0.0, sum);

    Candidate selected = candidates.front();

    double acc = 0.0;

    for (const auto& c : candidates)
    {
        acc += c.weight;

        if (r <= acc)
        {
            selected = c;
            break;
        }
    }

    child.genes.insert(
        child.genes.end(),
        parent1.genes.begin(),
        parent1.genes.begin() + selected.i + 1);

    child.genes.insert(
        child.genes.end(),
        parent2.genes.begin() + selected.j + 1,
        parent2.genes.end());

    // cycle repair
    std::unordered_map<const Stop*, int> visited;
    std::vector<ConnectionTime> repaired;

    for (const auto& gene : child.genes)
    {
        auto* stop = gene.to->getStop();

        auto it = visited.find(stop);

        if (it != visited.end())
        {
            repaired.resize(it->second + 1);

            visited.clear();

            for (int i = 0; i < repaired.size(); i++)
                visited[repaired[i].to->getStop()] = i;

            continue;
        }

        visited[stop] = repaired.size();
        repaired.push_back(gene);
    }

    child.genes = repaired;

    return child;
}

static void mutate(Individual& individual, const Network& network, const Stop* end)
{
    if (individual.genes.size() < 3)
        return;

    // =========================
    // Znajdź punkty przesiadek
    // =========================

    std::vector<int> transferPoints;

    for (int i = 1; i < individual.genes.size(); i++)
    {
        auto* prevTrip =
            individual.genes[i - 1].from->getTrip();

        auto* currTrip =
            individual.genes[i].from->getTrip();

        if (prevTrip != currTrip)
            transferPoints.push_back(i);
    }

    int mutationPoint;

    // =========================
    // Preferuj mutowanie przesiadek
    // =========================

    if (!transferPoints.empty() && randomDouble(0.0, 1.0) < 0.8)
        mutationPoint = transferPoints[randomInt(0,transferPoints.size() - 1)];
    else
        mutationPoint = randomInt(individual.genes.size() / 3, individual.genes.size() - 2);

    // =========================
    // Zachowaj początek trasy
    // =========================

    auto mutationStopTime = individual.genes[mutationPoint].from;

    individual.genes.resize(mutationPoint);

    std::unordered_set<const Stop*> visited;

    for (const auto& gene : individual.genes)
    {
        visited.insert(gene.from->getStop());
        visited.insert(gene.to->getStop());
    }

    // =========================
    // Odbudowa końcówki
    // =========================

    auto currentStopTime = mutationStopTime;

    while (currentStopTime && currentStopTime->getNextStopTime() && individual.genes.size() < MAX_PATH_LENGTH)
    {
        auto* currentStop = currentStopTime->getStop();

        if (currentStop == end)
            break;

        auto departures = network.getStopTimes(currentStop, currentStopTime->getTime());

        if (departures.empty())
            break;

        // =====================
        // Filtracja odwiedzonych
        // =====================

        std::vector<StopTime*> filtered;

        for (auto* dep : departures)
        {
            auto* nextStop = dep->getNextStopTime()->getStop();

            if (visited.find(nextStop) == visited.end())
                filtered.push_back(dep);
        }

        if (filtered.empty())
            filtered = departures;

        // =====================
        // Preferuj lepsze ETA
        // =====================

        std::vector<double> weights;

        double sum = 0.0;

        for (auto* dep : filtered)
        {
            auto* next = dep->getNextStopTime();
            double dist = geoDistance(next->getStop(), end);
            double wait =(dep->getTime() - currentStopTime->getTime()).count();
            double transferPenalty = 0.0;

            if (dep->getTrip() != currentStopTime->getTrip())
                transferPenalty = 8.0;

            double weight = 1.0 / (dist + 1.0) + exp(-0.08 * wait) - transferPenalty;
            weight = std::max(0.0001, weight);
            weights.push_back(weight);

            sum += weight;
        }

        // roulette wheel
        double r = randomDouble(0.0, sum);
        double accum = 0.0;

        StopTime* selected = filtered.back();

        for (int i = 0; i < filtered.size(); i++)
        {
            accum += weights[i];

            if (r <= accum)
            {
                selected = filtered[i];
                break;
            }
        }

        auto* next = selected->getNextStopTime();

        individual.genes.push_back({selected,next});

        visited.insert(next->getStop());
        currentStopTime = next;

        if (next->getStop() == end)
            break;
    }
}

static void repair(Individual& individual, const Network& network, const Stop* end)
{
    if (individual.genes.empty())
        return;

    auto* current = individual.genes.back().to;

    while ( current && current->getStop() != end && individual.genes.size() < MAX_PATH_LENGTH)
    {
        auto departures = network.getStopTimes( current->getStop(), current->getTime());

        if (departures.empty())
            break;

        auto next = chooseNextDeparture(departures, end, current->getTime(), network);
        auto* nextStop = next->getNextStopTime();

        individual.genes.push_back({ next, nextStop });
        current = nextStop;
    }
}

static std::string pathSignature(const Individual& individual)
{
    std::string sig;

    for (const auto& gene : individual.genes)
    {
        sig += gene.from->getTrip()->getId();

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

Individual generateIndividual(const Network& network, const Stop* start, const Stop* end, std::chrono::minutes departureTime)
{
    Individual individual;

    auto startOptions = network.getStopTimes(start, departureTime);

    std::unordered_set<const Stop*> visited;

    //wylosuj rozpoczecie podrozy
    auto currentStopTime = chooseNextDeparture(startOptions, end, departureTime, network);
    //weź następny przystanek z tej samej trasy
    auto nextStopTime = currentStopTime->getNextStopTime();
    individual.genes.push_back(ConnectionTime{ currentStopTime, nextStopTime });

    while (individual.genes.back().to->getStop() != end && individual.genes.size() < MAX_PATH_LENGTH)
    {
        if (currentStopTime == nullptr || nextStopTime == nullptr)
            break;

        //wez opcje kontynuacji podrozy z tego przystanku
        auto departureOptions = network.getStopTimes(nextStopTime->getStop(), nextStopTime->getTime());

        //jesli brak opcji, to przerwij
        if (departureOptions.empty())
            break;

        visited.insert(currentStopTime->getStop());
        visited.insert(nextStopTime->getStop());

        std::vector<StopTime*> filteredDepartureOptions;
        for (auto& el : departureOptions)
        {
            auto nextStop = el->getNextStopTime()->getStop();

            if (visited.find(nextStop) == visited.end())
                filteredDepartureOptions.push_back(el);
        }

        if (filteredDepartureOptions.empty())
            filteredDepartureOptions = departureOptions;

        //85% szans, że wybierzemy kontynuacje tej samej trasy
        auto it = std::find_if(filteredDepartureOptions.begin(), filteredDepartureOptions.end(),
            [&](const auto& elem)
            {
                return elem->getTrip() == currentStopTime->getTrip();
            });

        if (it != filteredDepartureOptions.end() && randomInt(0, 100) < std::max(85, (int)(95 - individual.genes.size())))
            currentStopTime = *it;
        else
            currentStopTime = chooseNextDeparture(filteredDepartureOptions, end, nextStopTime->getTime(), network);

        //weź następny przystanek z tej samej trasy
        nextStopTime = currentStopTime->getNextStopTime();
        individual.genes.push_back(ConnectionTime{ currentStopTime, nextStopTime });
    }

    return individual;
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
        population.push_back(generateIndividual(network, start, end, departureTime));
    }

    for (int generation = 0; generation < GENERATIONS; generation++)
    {
        // ocena
        for (auto& individual : population)
            evaluateIndividual(individual, departureTime, end);

        // sortowanie Pareto
        auto fronts = nonDominatedSort(population);

        for (const auto& front : fronts)
            computeCrowdingDistance(population, front);

        // potomstwo
        std::vector<Individual> offspring;

        while (offspring.size() < population.size())
        {
            auto parent1 = tournamentSelection(population, end);
            auto parent2 = tournamentSelection(population, end);

            auto child = crossover(parent1, parent2, end);

            if (randomDouble(0.0, 1.0) < MUTATION_RATE)
                mutate(child, network, end);

            repair(child, network, end);

            evaluateIndividual(child, departureTime, end);

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
        while (population.size() < POPULATION)
        {
            population.push_back(
                generateIndividual(network, start, end, departureTime));
        }
    }

    auto fronts = nonDominatedSort(population);

    std::vector<Path> results;

    for (int idx : fronts[0])
    {
        auto& el = population[idx];
        if (!el.isValid)
            continue;
        Path path(el.genes, el.arrivalTime, el.travelTime, el.waitingTime, el.cost, el.transfers);

        results.push_back(path);
    }

    std::sort(results.begin(), results.end(), [](const Path& p1, const Path& p2) {
        if(p1.getArrivalTime() == p2.getArrivalTime())
            return p1.getTransfers() < p2.getTransfers();
        return p1.getArrivalTime() < p2.getArrivalTime();
    });


    /*for (const auto& path : results)
    {
        std::cout << path << std::endl;
    }*/

    return results;
}
