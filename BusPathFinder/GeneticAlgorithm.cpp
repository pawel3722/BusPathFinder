#include <random>
#include <algorithm>
#include <iostream>

#include "GeneticAlgorithm.h"
#include "Functions.h"


#define POPULATION 100
#define GENERATIONS 150
#define MAX_PATH_LENGTH 50

#define MIN_TRANSFER_DURATION 3
#define MAX_DEPARTURES_PER_ROUTE 1

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

static StopTime* chooseNextDeparture(
    std::vector<StopTime*> departureOptions,
    const Stop* end,
    std::chrono::minutes arrival)
{
    std::vector<double> weights;
    double weightSum = 0.0;

    for (auto* dep : departureOptions)
    {
        //for debugging
        auto trip = dep->getTrip();
        auto tripName = trip->getLine() + " " + trip->getDirection();
        auto next = dep->getNextStopTime()->getStop()->getName();
        auto time = dep->getTime();

        double currentDist = geoDistance(dep->getStop(), end);
        double nextDist = geoDistance(dep->getNextStopTime()->getStop(), end);
        double dist = currentDist - nextDist;
        if (dist < 0)
            dist = 0.001;

        double wait = (dep->getTime().count() - arrival.count()) * 1.0;

        double weight = exp(dist) * exp(-0.1 * (wait - MIN_TRANSFER_DURATION));

        weights.push_back(weight);
        weightSum += weight;
    }

    double r = randomDouble(0.0, weightSum);
    double acc = 0.0;

    for (int i = 0; i < departureOptions.size(); i++)
    {
        acc += weights[i];

        if (r <= acc)
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
                (population[sorted[i + 1]].arrivalTime.count()
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

    population[sorted.front()].crowdingDistance = INFINITY;
    population[sorted.back()].crowdingDistance = INFINITY;

    minVal = population[sorted.front()].travelTime.count() * 1.0;
    maxVal = population[sorted.back()].travelTime.count() * 1.0;

    if (maxVal > minVal)
    {
        for (int i = 1; i < sorted.size() - 1; i++)
        {
            population[sorted[i]].crowdingDistance +=
                (population[sorted[i + 1]].travelTime.count()
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

    population[sorted.front()].crowdingDistance = INFINITY;
    population[sorted.back()].crowdingDistance = INFINITY;

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

Individual crossover(
    const Individual& parent1,
    const Individual& parent2,
    const Stop* end)
{
    Individual child;

    struct Candidate
    {
        int i;
        int j;
        double weight;
    };

    std::vector<Candidate> candidates;

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

static bool fixWaitingTimes(Individual& individual,
    const Network& network)
{
    
    std::vector<const StopTime*> transferPointArrs;
    std::vector<const StopTime*> transferPointDeps;
    std::vector<int> indices;
    Trip* currentTrip = nullptr;

    for (int i = 0; i < individual.genes.size(); i++)
    {
        if (currentTrip && currentTrip != individual.genes[i].from->getTrip())
        {
            transferPointArrs.push_back(individual.genes[i - 1].to);
            transferPointDeps.push_back(individual.genes[i].from);
            indices.push_back(i);
        }
        currentTrip = individual.genes[i].from->getTrip();
    }

    for (int i = static_cast<int>(transferPointDeps.size()) - 1; i >= 0; i--)
    {
        auto ed = network.getEarlierDeparture(transferPointDeps[i]);
        if (ed == nullptr || transferPointArrs[i]->getTime().count() + MIN_TRANSFER_DURATION > ed->getTime().count())
            continue;
        int maxIndex = i == transferPointDeps.size() - 1 ? individual.genes.size() : indices[i + 1];
        int minIndex = indices[i];

        //TODO
        for (int j = minIndex; j < maxIndex; j++)
        {
            auto from = individual.genes[j].from;
            auto to = individual.genes[j].to;
            individual.genes[j] = { network.getEarlierDeparture(from), network.getEarlierDeparture(to) };
        }

        for (int k = 1; k < individual.genes.size(); k++)
        {
            if (individual.genes[k - 1].to->getTime() > individual.genes[k].from->getTime() || individual.genes[k - 1].to->getStop() != individual.genes[k].from->getStop())
                int x = 9;
        }


        return true;
    }

    for (int i = 0; i < transferPointDeps.size(); i++)
    {
        auto la = network.getLaterDeparture(transferPointArrs[i]);
        if (la == nullptr || la->getTime().count() + MIN_TRANSFER_DURATION > transferPointDeps[i]->getTime().count())
            continue;
        int minIndex = i ? indices[i - 1] : 0;
        int maxIndex = indices[i];

        //TODO
        for (int j = minIndex; j < maxIndex; j++)
        {
            auto from = individual.genes[j].from;
            auto to = individual.genes[j].to;
            individual.genes[j] = { network.getLaterDeparture(from), network.getLaterDeparture(to) };
        }

        for (int k = 1; k < individual.genes.size(); k++)
        {
            if (individual.genes[k - 1].to->getTime() > individual.genes[k].from->getTime() || individual.genes[k - 1].to->getStop() != individual.genes[k].from->getStop())
                int x = 9;
        }

        return true;
    }
    return false;
}

static bool skipConnection(Individual& individual, const Network& network)
{
    Trip* currentTrip = nullptr;
    std::vector<const StopTime*> tripEntryPoints;

    for (const auto& el : individual.genes)
    {
        if (el.from->getTrip() != currentTrip)
        {
            tripEntryPoints.push_back(el.from);
            currentTrip = el.from->getTrip();
        }
    }

    for (int i = 0; i < tripEntryPoints.size(); i++)
    {
        for (int j = tripEntryPoints.size() - 1; j >= i + 2; j--)
        {
            auto commonStop = network.getCommonStop(tripEntryPoints[i]->getTrip(), tripEntryPoints[j]->getTrip(),tripEntryPoints[i], tripEntryPoints[j]);
            if (commonStop == nullptr)
                continue;

            auto trip1 = tripEntryPoints[i]->getTrip();
            auto& st1 = trip1->getStopTimes();
            int index1 = tripEntryPoints[i]->getIndexInRoute();

            auto trip2 = tripEntryPoints[j]->getTrip();
            auto& st2 = trip2->getStopTimes();
            int index2 = commonStop->getIndexInRoute();

            if (tripEntryPoints[j]->getIndexInRoute() < commonStop->getIndexInRoute())
                continue;

            std::vector<ConnectionTime> newGenes;

            for (const auto& el : individual.genes)
            {
                if (el.from == tripEntryPoints[i])
                    break;
                newGenes.push_back(el);
            }

            while (index1 + 1 < st1.size() && st1[index1]->getStop() != commonStop->getStop())
            {
                newGenes.push_back({ st1[index1], st1[ ++index1] });
            }

            if (st1[index1]->getTime().count() + MIN_TRANSFER_DURATION > st2[index2]->getTime().count())
                continue;

            while (index2 + 1 < st2.size() && st2[index2]->getStop() != tripEntryPoints[j]->getStop())
            {
                newGenes.push_back({ st2[index2], st2[++index2] });
            }

            for (auto it = std::find_if(individual.genes.begin(), individual.genes.end(), [&](const ConnectionTime& ct) {return ct.from == tripEntryPoints[j];}); it != individual.genes.end(); it++)
            {
                newGenes.push_back(*it);
            }

            for (int k = 1; k < newGenes.size(); k++)
            {
                if (newGenes[k - 1].to->getTime() > newGenes[k].from->getTime() || newGenes[k - 1].to->getStop() != newGenes[k].from->getStop())
                    int x = 9;
            }

            individual.genes = newGenes;
            return true;
        }
    }
    return false;
}

static void mutate(
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

    while (individual.genes.back().to->getStop() != end && individual.genes.size() < MAX_PATH_LENGTH)
    {
        auto departures = network.getStopTimes(
            stopTime->getStop(),
            stopTime->getTime(),
            stopTime->getTrip(),
            MIN_TRANSFER_DURATION,
            MAX_DEPARTURES_PER_ROUTE);

        if (departures.empty())
            break;

        auto current = chooseNextDeparture(departures, end, individual.genes.back().to->getTime());
        auto next = current->getNextStopTime();

        if (!individual.genes.empty() && current->getTime() < individual.genes.back().to->getTime())
            int x = 9;

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
        sig += gene.from->getTrip()->getId();

        sig += "|";

        sig += std::to_string(
            gene.from->getStop()->getId());

        sig += "|";

        sig += std::to_string(
            gene.from->getTime().count());

        sig += "->";
    }
    if (individual.genes.size() > 0)
    {
        auto gene = individual.genes.back();
        
        sig += gene.to->getTrip()->getId();

        sig += "|";

        sig += std::to_string(
            gene.to->getStop()->getId());

        sig += "|";

        sig += std::to_string(
            gene.to->getTime().count());
    }

    return sig;
}

std::vector<Path> GeneticAlgorithm::findPath(const Network& network, const Stop* start, const Stop* end, std::chrono::minutes departureTime)
{
    auto startOptions = network.getStopTimes(start, departureTime);

    if (startOptions.empty())
        return { Path("No departures from starting stop!") };
    if(start == end)
        return { Path("Starting stop equals destination!") };

    // populacja
    std::vector<Individual> population;

    for (int i = 0; i < POPULATION; i++)
    {
        Individual individual;

        std::unordered_set<const Stop*> visited;

        //wylosuj rozpoczecie podrozy
        auto currentStopTime = chooseNextDeparture(startOptions, end, departureTime);
        auto nextStopTime = currentStopTime->getNextStopTime();
        if (!individual.genes.empty() && currentStopTime->getTime() < individual.genes.back().to->getTime())
            int x = 9;

        individual.genes.push_back(ConnectionTime{ currentStopTime, nextStopTime });

        while (individual.genes.back().to->getStop() != end && individual.genes.size() < MAX_PATH_LENGTH)
        {
            if (!currentStopTime || !nextStopTime)
                break;

            //wez opcje kontynuacji podrozy z tego przystanku
            auto departureOptions = network.getStopTimes(nextStopTime->getStop(), nextStopTime->getTime(), nextStopTime->getTrip(), MIN_TRANSFER_DURATION, MAX_DEPARTURES_PER_ROUTE);

            visited.insert(currentStopTime->getStop());
            visited.insert(nextStopTime->getStop());

            std::vector<StopTime*> filteredDepartureOptions;
            for (auto& el : departureOptions)
                if (visited.find(el->getNextStopTime()->getStop()) == visited.end())
                    filteredDepartureOptions.push_back(el);

            //jesli brak opcji, to przerwij
            if (filteredDepartureOptions.empty())
                break;

            //85% szans, że wybierzemy kontynuacje tej samej trasy
            auto it = std::find_if(filteredDepartureOptions.begin(), filteredDepartureOptions.end(),
                [&](const auto& elem)
                {
                    return elem->getTrip() == currentStopTime->getTrip();
                });

            if (it != filteredDepartureOptions.end() && randomInt(0, 100) < 95)
                currentStopTime = *it;
            else
                currentStopTime = chooseNextDeparture(filteredDepartureOptions, end, nextStopTime->getTime());

            //weź następny przystanek z tej samej trasy
            nextStopTime = currentStopTime->getNextStopTime();
            
            
            if (!individual.genes.empty() && currentStopTime->getTime() < individual.genes.back().to->getTime())
                int x = 9;


            individual.genes.push_back(ConnectionTime{ currentStopTime, nextStopTime });
        }

        population.push_back(individual);
    }

    for (int generation = 0; generation < GENERATIONS; generation++)
    {





        // ocena
        for (auto& individual : population)
        {
            evaluateIndividual(individual, departureTime, end);
            //sprobuj opoznic rozpoczecie podrozy
            /*if (individual.transfers > 0 && individual.isValid)
            {
                if(delayStart(individual, network));
                    evaluateIndividual(individual, departureTime, end);
            }*/
        }

        // sortowanie Pareto
        auto fronts = nonDominatedSort(population);

        for (const auto& front : fronts)
            computeCrowdingDistance(population, front);

        // potomstwo
        std::vector<Individual> offspring;

        std::unordered_set<std::string> offspringSignatures;

        while (offspring.size() < population.size())
        {
            auto parent1 = tournamentSelection(population, end);
            auto parent2 = tournamentSelection(population, end);

            auto child = crossover(parent1, parent2, end);

            if (randomDouble(0.0, 1.0) < 0.2)
                mutate(child, network, end);

            evaluateIndividual(child, departureTime, end);

            if (child.isValid && child.transfers > 0 && randomDouble(0.0, 1.0) < 0.5)
            {
                fixWaitingTimes(child, network);
                evaluateIndividual(child, departureTime, end);
            }

            if (child.isValid && child.transfers >= 2 && randomDouble(0.0, 1.0) < 0.3)
            {
                skipConnection(child, network);
                evaluateIndividual(child, departureTime, end);
            }

            evaluateIndividual(child, departureTime, end);

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
        auto& el = population[idx];
        if (!el.isValid)
            continue;
        results.push_back(Path(
            el.genes,
            el.arrivalTime,
            el.travelTime,
            el.waitingTime,
            el.cost,
            el.transfers));
    }

    std::sort(results.begin(), results.end(), [&](const Path& p1, const Path& p2) {
        if (p1.getArrivalTime() == p2.getArrivalTime())
            return p1.getTransfers() < p2.getTransfers();
        return p1.getArrivalTime() < p2.getArrivalTime();
        });

    if (results.empty())
        results.push_back(Path("Journey from start to end was not found!"));
    else
    {
        results.erase(std::unique(results.begin(), results.end()), results.end());
    }

    return results;
}
