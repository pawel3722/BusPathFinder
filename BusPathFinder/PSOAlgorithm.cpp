#include <vector>
#include <unordered_set>
#include <algorithm>
#include <chrono>

#include "PSOAlgorithm.h"
#include "Functions.h"

#define SWARM_SIZE 100
#define ITERATIONS 100
#define MAX_PATH_LENGTH 70
#define MAX_ARCHIVE_SIZE 100

#define MUTATION_PROB 0.20
#define PBEST_PROB 0.45
#define GBEST_PROB 0.45

#define MIN_TRANSFER_DURATION 3
#define MAX_DEPARTURES_PER_ROUTE 3
#define SAME_TRIP_PROB 0.95

enum Objective
{
    ARRIVAL,
    TRAVEL,
    WAITING,
    TRANSFERS,
    COST
};

struct Particle
{
    std::vector<ConnectionTime> path;
    std::vector<ConnectionTime> personalBest;

    bool isValid = false;

    Objective objective;

    std::chrono::minutes arrivalTime = std::chrono::minutes::max();
    std::chrono::minutes travelTime = std::chrono::minutes::max();
    std::chrono::minutes waitingTime = std::chrono::minutes(0);

    double cost = 0.0;
    double distanceToGoal = 1e9;

    int transfers = 0;
};

static double calculateCost(const ConnectionTime& connection)
{
    return 0.0;
}

static bool dominates(const Particle& a, const Particle& b)
{
    if (!b.isValid)
        return a.isValid;

    if (!a.isValid || a.arrivalTime > b.arrivalTime || a.travelTime > b.travelTime || a.waitingTime > b.waitingTime || a.cost > b.cost || a.transfers > b.transfers)
        return false;

    if (a.arrivalTime < b.arrivalTime || a.travelTime < b.travelTime || a.waitingTime < b.waitingTime || a.cost < b.cost || a.transfers < b.transfers)
        return true;

    return false;
}

static std::vector<Trip*> buildTripSequence(const std::vector<ConnectionTime>& path)
{
    std::vector<Trip*> result;
    Trip* previous = nullptr;

    for (const auto& edge : path)
    {
        auto* trip = edge.from->getTrip();

        if (trip != previous)
        {
            result.push_back(trip);
            previous = trip;
        }
    }

    return result;
}

static double pathSimilarity(const Particle& a, const Particle& b)
{
    auto tripsA = buildTripSequence(a.path);
    auto tripsB = buildTripSequence(b.path);

    if (tripsA.empty() || tripsB.empty())
        return 0.0;

    std::unordered_set<Trip*> setB(tripsB.begin(), tripsB.end());

    int common = 0;

    for (auto* trip : tripsA)
    {
        if (setB.find(trip) != setB.end())
            common++;
    }

    return common * 1.0 / std::max(tripsA.size(), tripsB.size());
}

static double heuristicValue(Objective objective,
    StopTime* dep,
    const Stop* end,
    std::chrono::minutes arrival,
    StopTime* previous)
{
    double currentDist = geoDistance(dep->getStop(), end);
    double nextDist = geoDistance(dep->getNextStopTime()->getStop(), end);
    double wait = (dep->getTime() - arrival).count();
    bool transfer = previous && previous->getTrip() != dep->getTrip();
    double progress = currentDist - nextDist;
    double transferPenalty = transfer ? 0.7 : 1.0;

    switch (objective)
    {
    case ARRIVAL:
    case TRAVEL:
        return exp(progress) * exp(-0.1 * (wait - MIN_TRANSFER_DURATION)) * transferPenalty;

    case WAITING:
        return exp(progress) * exp(-0.3 * (wait - MIN_TRANSFER_DURATION)) * transferPenalty;

    case COST:
        return 1.0 / (calculateCost({ dep, dep->getNextStopTime() }) + 1.0);

    case TRANSFERS:
        return transfer ? 0.2 : 3.0;
    }

    return 1.0;
}

static bool containsEdge(const std::vector<ConnectionTime>& path,
    const ConnectionTime& edge)
{
    return std::find(path.begin(), path.end(), edge) != path.end();
}

static StopTime* chooseNextDeparture(
    const std::vector<StopTime*>& departureOptions,
    const Stop* end,
    std::chrono::minutes arrival,
    StopTime* previous,
    Objective objective,
    const std::vector<ConnectionTime>* pbest = nullptr,
    const std::vector<ConnectionTime>* gbest = nullptr)
{

    double epsilon = 0.1;

    if (randomDouble(0.0, 1.0) < epsilon)
    {
        return departureOptions[randomInt(0, departureOptions.size() - 1)];
    }

    std::vector<double> weights;

    double sum = 0.0;

    for (auto* dep : departureOptions)
    {
        if (!dep || !dep->getNextStopTime())
        {
            weights.push_back(0.0);
            continue;
        }

        auto edge = ConnectionTime{ dep, dep->getNextStopTime() };
        double weight = heuristicValue(objective, dep, end, arrival, previous);

        if (pbest && containsEdge(*pbest, edge))
            weight *= 1.5;

        if (gbest && containsEdge(*gbest, edge))
            weight *= 2.0;

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

static Particle buildParticle(const Network& network, const Stop* start, const Stop* end, std::chrono::minutes departureTime)
{
    Particle particle;

    auto objective = (Objective)randomInt(0, 3);
    particle.objective = objective;

    auto startOptions = network.getStopTimes(start, departureTime, nullptr, 0, MAX_DEPARTURES_PER_ROUTE);

    if (startOptions.empty())
        return particle;

    auto* current = chooseNextDeparture(startOptions, end, departureTime, nullptr, objective);
    auto* next = current ? current->getNextStopTime() : nullptr;

    if (!current || !next)
        return particle;

    particle.path.push_back({ current, next });

    std::unordered_set<const Stop*> visited;

    while (next->getStop() != end && particle.path.size() < MAX_PATH_LENGTH)
    {
        visited.insert(current->getStop());

        auto nextDepartures = network.getStopTimes(next->getStop(), next->getTime(), next->getTrip(), MIN_TRANSFER_DURATION, MAX_DEPARTURES_PER_ROUTE);

        if (nextDepartures.empty())
            break;

        std::vector<StopTime*> filtered;

        for (auto* dep : nextDepartures)
        {
            if (!dep || !dep->getNextStopTime())
                continue;

            if (visited.find(dep->getNextStopTime()->getStop()) != visited.end())
                continue;

            filtered.push_back(dep);
        }

        if (filtered.empty())
            filtered = nextDepartures;

        auto sameTripIt = std::find_if(filtered.begin(), filtered.end(),
            [&](auto* dep)
            {
                return dep->getTrip() == current->getTrip();
            });

        if (sameTripIt != filtered.end() && randomDouble(0.0, 1.0) < SAME_TRIP_PROB)
            current = *sameTripIt;
        else
            current = chooseNextDeparture(filtered, end, next->getTime(), current, objective);

        if (!current || !current->getNextStopTime())
            break;

        next = current->getNextStopTime();

        particle.path.push_back({ current, next });
    }

    return particle;
}

static void evaluateParticle(Particle& particle, std::chrono::minutes departureTime, const Stop* end)
{
    if (particle.path.empty())
        return;

    particle.isValid = particle.path.back().to->getStop() == end;

    if (!particle.isValid)
    {
        double minDistance = geoDistance(particle.path.front().from->getStop(), end);

        for (const auto& edge : particle.path)
        {
            minDistance = std::min(minDistance, geoDistance(edge.to->getStop(), end));
        }

        particle.distanceToGoal = minDistance;

        return;
    }

    auto startTime = particle.path.front().from->getTime();
    auto endTime = particle.path.back().to->getTime();

    particle.arrivalTime = endTime;
    particle.travelTime = endTime - startTime;
    particle.waitingTime = std::chrono::minutes(0);

    particle.transfers = 0;
    particle.cost = 0.0;

    Trip* previousTrip = nullptr;
    auto arrival = departureTime;

    for (const auto& edge : particle.path)
    {
        auto* trip = edge.from->getTrip();

        if (previousTrip && previousTrip != trip)
        {
            particle.transfers++;

            auto wait = edge.from->getTime() - arrival;

            if (wait.count() > 0)
                particle.waitingTime += wait;
        }

        particle.cost += calculateCost(edge);

        previousTrip = trip;
        arrival = edge.to->getTime();
    }
}

static void updateArchive(std::vector<Particle>& archive, const Particle& candidate)
{
    if (candidate.path.empty())
        return;

    for (const auto& particle : archive)
    {
        if (dominates(particle, candidate))
            return;
    }

    for (auto it = archive.begin(); it != archive.end();)
    {
        if (dominates(candidate, *it))
            it = archive.erase(it);
        else
            ++it;
    }

    for (const auto& particle : archive)
    {
        if (pathSimilarity(candidate, particle) > 0.90)
            return;
    }

    archive.push_back(candidate);

    if (archive.size() > MAX_ARCHIVE_SIZE)
    {
        archive.erase(archive.begin() + randomInt(0, archive.size() - 1));
    }
}

static const Particle& selectLeader(const std::vector<Particle>& archive, Objective objective)
{
    std::vector<const Particle*> candidates;

    for (const auto& particle : archive)
    {
        if (particle.isValid)
            candidates.push_back(&particle);
    }

    if (candidates.empty())
    {
        return archive[randomInt(0, archive.size() - 1)];
    }

    std::sort(candidates.begin(), candidates.end(),
        [&](const Particle* a, const Particle* b)
        {
            switch (objective)
            {
            case ARRIVAL:
                return a->arrivalTime < b->arrivalTime;

            case TRAVEL:
                return a->travelTime < b->travelTime;

            case WAITING:
                return a->waitingTime < b->waitingTime;

            case TRANSFERS:
                return a->transfers < b->transfers;

            case COST:
                return a->cost < b->cost;
            }

            return false;
        });

    int eliteCount = std::max(1, (int)(candidates.size() * 0.3));

    return *candidates.front();
}

static void rerouteFromIndex(Particle& particle, const Particle& leader, const Network& network, const Stop* end, int splitIndex)
{
    if (particle.path.empty())
        return;

    splitIndex = std::clamp(splitIndex, 0, (int)particle.path.size() - 1);

    particle.path.resize(splitIndex + 1);

    auto* current = particle.path.back().to;

    std::unordered_set<const Stop*> visited;

    for (const auto& edge : particle.path)
    {
        visited.insert(edge.from->getStop());
    }

    while (particle.path.size() < MAX_PATH_LENGTH)
    {
        if (!current || current->getStop() == end)
            break;

        auto departures = network.getStopTimes(current->getStop(), current->getTime(), current->getTrip(), MIN_TRANSFER_DURATION, MAX_DEPARTURES_PER_ROUTE);

        if (departures.empty())
            break;

        std::vector<StopTime*> filtered;

        for (auto* dep : departures)
        {
            if (!dep || !dep->getNextStopTime())
                continue;

            if (visited.find(dep->getNextStopTime()->getStop()) != visited.end())
                continue;

            filtered.push_back(dep);
        }

        if (filtered.empty())
            filtered = departures;

        auto sameTripIt = std::find_if(filtered.begin(), filtered.end(),
            [&](auto* dep)
            {
                return dep->getTrip() == current->getTrip();
            });

        auto currentStop = *current;
        StopTime* ptr = &currentStop;

        if (sameTripIt != filtered.end() && randomDouble(0.0, 1.0) < SAME_TRIP_PROB)
            current = *sameTripIt;
        else
            current = chooseNextDeparture(filtered, end, current->getTime(), 
                ptr, particle.objective, &particle.personalBest, &leader.path);
            
        if (!current || !current->getNextStopTime())
            break;
        
        auto next = current->getNextStopTime();

        particle.path.push_back({ current, next });

        current = next;
        visited.insert(current->getStop());
    }
}

static void followPath(Particle& particle, const Particle&leader, const std::vector<ConnectionTime>& target, const Network& network, const Stop* end)
{
    if (particle.path.empty() || target.empty())
        return;

    int split = randomInt(0, std::min((int)particle.path.size() - 1, (int)target.size() - 1));

    auto targetStop = target[split].from;

    int currentIndex = -1;

    for (int i = 0; i < particle.path.size(); i++)
    {
        if (particle.path[i].to->getStop() == targetStop->getStop() 
         && particle.path[i].to->getTime() < targetStop->getTime())
        {
            currentIndex = i;
            break;
        }
    }

    if(currentIndex == -1 && particle.path.front().from->getStop() != targetStop->getStop())
    {
        rerouteFromIndex(particle, leader, network, end, randomInt(0, particle.path.size() - 1));
        return;
    }

    particle.path.resize(currentIndex + 1);

    for (int i = split; i < target.size(); i++)
    {
        particle.path.push_back(target[i]);

        if (particle.path.size() >= MAX_PATH_LENGTH)
            break;
    }
}

std::vector<Path> PSOAlgorithm::findPath(const Network& network, const Stop* start, const Stop* end, std::chrono::minutes departureTime)
{
    if (start == end)
    {
        return { Path("Starting stop equals destination!") };
    }

    std::vector<Particle> swarm;
    std::vector<Particle> archive;

    for (int i = 0; i < SWARM_SIZE; i++)
    {
        auto particle = buildParticle(network, start, end, departureTime);

        evaluateParticle(particle, departureTime, end);

        particle.personalBest = particle.path;

        swarm.push_back(particle);

        updateArchive(archive, particle);
    }

    for (int iteration = 0; iteration < ITERATIONS; iteration++)
    {
        if (archive.empty())
            break;

        for (auto& particle : swarm)
        {
            const auto& leader = selectLeader(archive, particle.objective);
            double r = randomDouble(0.0, 1.0);

            if (r < PBEST_PROB && !particle.personalBest.empty())
            {
                followPath(particle, leader, particle.personalBest, network, end);
            }
            else if (r < PBEST_PROB + GBEST_PROB)
            {
                followPath(particle, leader, leader.path, network, end);
            }

            if (randomDouble(0.0, 1.0) < MUTATION_PROB)
            {
                rerouteFromIndex(particle, leader, network, end, randomInt(0, particle.path.size() - 1));
            }

            evaluateParticle(particle, departureTime, end);

            Particle current;
            current.path = particle.path;
            current.isValid = particle.isValid;
            current.arrivalTime = particle.arrivalTime;
            current.travelTime = particle.travelTime;
            current.waitingTime = particle.waitingTime;
            current.cost = particle.cost;
            current.transfers = particle.transfers;

            Particle best;
            best.path = particle.personalBest;
            best.isValid = particle.isValid;
            best.arrivalTime = particle.arrivalTime;
            best.travelTime = particle.travelTime;
            best.waitingTime = particle.waitingTime;
            best.cost = particle.cost;
            best.transfers = particle.transfers;

            if (particle.personalBest.empty() || dominates(current, best))
            {
                particle.personalBest = particle.path;
            }

            updateArchive(archive, particle);
        }
    }

    std::vector<Path> results;

    for (const auto& particle : archive)
    {
        if (!particle.isValid)
            continue;

        results.push_back(Path(
            particle.path,
            particle.arrivalTime,
            particle.travelTime,
            particle.waitingTime,
            particle.cost,
            particle.transfers));
    }

    std::sort(results.begin(), results.end(),
        [](const Path& a, const Path& b)
        {
            if (a.getArrivalTime() == b.getArrivalTime())
                return a.getTransfers() < b.getTransfers();

            return a.getArrivalTime() < b.getArrivalTime();
        });

    if (results.empty())
    {
        results.push_back(Path("Journey not found!"));
    }

    return results;
}