#include <unordered_map>
#include <vector>
#include <random>
#include <algorithm>
#include <unordered_set>

#include "ACOAlgorithm.h"
#include "Functions.h"

#define ANT_COUNT 100
#define ITERATIONS 100
#define MAX_PATH_LENGTH 50
#define MAX_PARETO_SIZE 100

#define ALPHA 0.7
#define BETA 3.0
#define EVAPORATION 0.15

struct Ant
{
    std::vector<ConnectionTime> path;

    bool isValid = false;

    std::chrono::minutes arrivalTime = std::chrono::minutes::max();
    std::chrono::minutes travelTime = std::chrono::minutes::max();
    std::chrono::minutes waitingTime = std::chrono::minutes(0);

    double cost = 0.0;
    double distanceToGoal = 1e9;

    int transfers = 0;

    bool operator==(const Ant& other) const
    {
        return path == other.path;
    }
};

static std::string pathSignature(const Ant& ant)
{
    std::string sig;

    for (const auto& node : ant.path)
    {
        sig += node.from->getTrip()->getId();

        sig += "|";

        sig += std::to_string(node.from->getStop()->getId());

        sig += "|";

        sig += std::to_string(node.from->getTime().count());

        sig += "->";
    }
    if (ant.path.size() > 0)
    {
        auto node = ant.path.back();

        sig += node.to->getTrip()->getId();

        sig += "|";

        sig += std::to_string(
            node.to->getStop()->getId());

        sig += "|";

        sig += std::to_string(
            node.to->getTime().count());
    }

    return sig;
}

struct AntHash
{
    size_t operator()(const Ant& c) const
    {
        size_t h1 =
            std::hash<std::string>()(pathSignature(c));

        return h1;
    }
};

struct ConnectionTimeHash
{
    size_t operator()(const ConnectionTime& c) const
    {
        size_t h1 =
            std::hash<const void*>()(c.from);

        size_t h2 =
            std::hash<const void*>()(c.to);

        return h1 ^ (h2 << 1);
    }
};

static bool dominates(const Ant& a, const Ant& b)
{
    if (b.path.size() == 0)
        return true;

    if (!a.isValid && !b.isValid)
        return false;

    if (!a.isValid || a.arrivalTime > b.arrivalTime || a.travelTime > b.travelTime || a.waitingTime > b.waitingTime || a.cost > b.cost || a.transfers > b.transfers)
        return false;

    if (!b.isValid || a.arrivalTime < b.arrivalTime || a.travelTime < b.travelTime || a.waitingTime < b.waitingTime || a.cost < b.cost || a.transfers < b.transfers)
        return true;

    return false;
}

static void updateParetoArchive(std::unordered_set<Ant, AntHash>& archive, const Ant& candidate)
{
    /*if (!candidate.isValid)
        return;*/

    for (const auto& ant : archive)
    {
        if (dominates(ant, candidate))
            return;
    }

    for (auto it = archive.begin(); it != archive.end(); )
    {
        if (dominates(candidate, *it))
            it = archive.erase(it);
        else
            ++it;
    }

    archive.insert(candidate);

    /*if (archive.size() > MAX_PARETO_SIZE)
    {
        std::shuffle(archive.begin(), archive.end(), rng);
        archive.resize(MAX_PARETO_SIZE);
    }*/
}

static double calculateCost(const ConnectionTime& connection)
{
    return 0.0;

    auto duration = connection.to->getTime() - connection.from->getTime();

    return duration.count() * 0.15;
}

static StopTime* chooseNextDeparture(const std::vector<StopTime*>& departureOptions, const Stop* end,
    std::chrono::minutes arrival,
    const std::unordered_map<ConnectionTime, double, ConnectionTimeHash>& pheromones)
{
    std::vector<double> weights;
    double sum = 0.0;

    for (auto* dep : departureOptions)
    {
        auto connection = ConnectionTime{ dep, dep->getNextStopTime() };
        double pheromone = 1.0;
        auto it = pheromones.find(connection);

        if (it != pheromones.end())
            pheromone = it->second;

        double currentDist = geoDistance(dep->getStop(), end);
        double nextDist = geoDistance(dep->getNextStopTime()->getStop(), end);
        double dist = nextDist - currentDist;

        double wait = (dep->getTime().count() - arrival.count()) * 1.0;

        double heuristic = 1.0 / (dist + 1.0) * exp(-0.15 * wait);

        double weight = pow(pheromone, ALPHA) * pow(heuristic, BETA);
        weight = std::max(0.00001, weight);

        weights.push_back(weight);
        sum += weight;
    }

    double r = randomDouble(0.0, sum);
    double acc = 0.0;

    for (int i = 0; i < departureOptions.size(); i++)
    {
        acc += weights[i];

        if (r <= acc)
            return departureOptions[i];
    }

    return departureOptions.back();
}

static Ant buildAnt(const Network& network, const Stop* start, const Stop* end,
    std::chrono::minutes departureTime,
    const std::unordered_map<ConnectionTime, double, ConnectionTimeHash>& pheromones)
{
    Ant ant;
    auto startOptions = network.getStopTimes(start, departureTime);

    if (startOptions.empty())
        return ant;

    std::unordered_set<const Stop*> visited;

    auto currentStopTime = chooseNextDeparture(startOptions, end, departureTime, pheromones);
    auto nextStopTime = currentStopTime->getNextStopTime();

    if (!currentStopTime || !nextStopTime)
        return ant;

    ant.path.push_back({ currentStopTime, nextStopTime });

    while (ant.path.back().to->getStop() != end && ant.path.size() < MAX_PATH_LENGTH)
    {
        if (!currentStopTime || !nextStopTime)
            break;

        auto departureOptions = network.getStopTimes(nextStopTime->getStop(), nextStopTime->getTime());

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
            currentStopTime = chooseNextDeparture(filteredDepartureOptions, end, nextStopTime->getTime(), pheromones);

        //weź następny przystanek z tej samej trasy
        nextStopTime = currentStopTime->getNextStopTime();
        ant.path.push_back(ConnectionTime{ currentStopTime, nextStopTime });
    }

    return ant;
}

static void evaluateAnt(Ant& ant, std::chrono::minutes departureTime, const Stop* end)
{
    if (ant.path.empty())
    {
        ant.isValid = false;
        return;
    }

    ant.isValid = ant.path.back().to->getStop() == end;

    if (!ant.isValid)
    {
        double minDistance = geoDistance(ant.path.front().from->getStop(), end);
        for (const auto& el : ant.path)
        {
            auto dist = geoDistance(el.to->getStop(), end);
            if (dist < minDistance)
                minDistance = dist;
        }
        ant.distanceToGoal = minDistance;
        ant.arrivalTime = std::chrono::minutes::max() / 2;
        ant.travelTime = std::chrono::minutes::max() / 2;
        ant.waitingTime = std::chrono::minutes::max() / 2;
        ant.cost = 1e9;
        ant.transfers = 1e9;
        return;
    }

    auto startTime = ant.path.front().from->getTime();
    auto endTime = ant.path.back().to->getTime();

    ant.arrivalTime = endTime;
    ant.travelTime = std::chrono::duration_cast<std::chrono::minutes>(endTime - startTime);
    ant.waitingTime = std::chrono::minutes(0);
    ant.transfers = 0;
    ant.cost = 0.0;

    Trip* previousTrip = nullptr;
    auto arrival = departureTime;

    for (const auto& connection : ant.path)
    {
        auto trip = connection.from->getTrip();

        if (previousTrip && previousTrip != trip)
        {
            ant.transfers++;
            ant.waitingTime += (connection.from->getTime() - arrival);
        }

        // TODO:
        // dodaj wyliczanie kosztu
        // np. strefy + linia pospieszna

        previousTrip = trip;
        arrival = connection.to->getTime();
    }
}

static void evaporate(std::unordered_map<ConnectionTime, double, ConnectionTimeHash>& pheromones)
{
    for (auto& p : pheromones)
    {
        p.second *= (1.0 - EVAPORATION);
        p.second = std::max(0.0001, p.second);
    }
}

static void reinforce(const std::unordered_set<Ant, AntHash>& archive,
    std::unordered_map<ConnectionTime, double, ConnectionTimeHash>& pheromones)
{
    for (const auto& ant : archive)
    {
        double reward = 1.0;

        if (!ant.isValid)
            reward += 1000.0 / (ant.distanceToGoal + 1.0);
        else
        {
            reward += 10000.0 / (ant.arrivalTime.count() + 1.0);
            reward += 10000.0 / (ant.travelTime.count() + 1.0);
            reward += 10000.0 / (ant.waitingTime.count() + 1.0);
            reward += 10000.0 / (ant.cost + 1.0);
            reward += 10000.0 / (ant.transfers + 1.0);
        }

        for (const auto& gene : ant.path)
        {
            pheromones[gene] += 0.0001 * reward;
        }
    }
}

std::vector<Path> ACOAlgorithm::findPath(const Network& network, const Stop* start,
    const Stop* end, std::chrono::minutes departureTime)
{
    std::unordered_map<ConnectionTime, double, ConnectionTimeHash> pheromones;

    std::unordered_set<Ant, AntHash> paretoArchive;

    for (int iteration = 0; iteration < ITERATIONS; iteration++)
    {
        std::vector<Ant> ants;

        for (int i = 0; i < ANT_COUNT; i++)
        {
            auto ant = buildAnt(network, start, end, departureTime, pheromones);

            evaluateAnt(ant, departureTime, end);

            ants.push_back(ant);
        }

        evaporate(pheromones);

        for (const auto& ant : ants)
        {
            updateParetoArchive(paretoArchive, ant);
        }

        reinforce(paretoArchive, pheromones);
    }

    std::vector<Path> results;

    for (const auto& ant : paretoArchive)
    {
        if (!ant.isValid)
            continue;
        results.push_back(Path(
            ant.path,
            ant.arrivalTime,
            ant.travelTime,
            ant.waitingTime,
            ant.cost,
            ant.transfers));
    }

    std::sort(results.begin(), results.end(), [](const Path& p1, const Path& p2) {
        if (p1.getArrivalTime() == p2.getArrivalTime())
            return p1.getTransfers() < p2.getTransfers();
        return p1.getArrivalTime() < p2.getArrivalTime();
        });

    return results;
}