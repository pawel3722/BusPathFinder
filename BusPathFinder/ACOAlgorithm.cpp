#include <unordered_map>
#include <random>
#include <algorithm>
#include <unordered_set>
#include <array>
#include <fstream>

#include "ACOAlgorithm.h"
#include "Functions.h"

ACOAlgorithm::ACOAlgorithm(const std::string& configFilePath)
{
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

    ANT_COUNT = std::stoi(params.at("ANT_COUNT"));
    ITERATIONS = std::stoi(params.at("ITERATIONS"));
    MAX_PATH_LENGTH = std::stoi(params.at("MAX_PATH_LENGTH"));

    MIN_TRANSFER_TIME = std::stoi(params.at("MIN_TRANSFER_TIME"));
    MAX_DEPARTURES_PER_ROUTE = std::stoi(params.at("MAX_DEPARTURES_PER_ROUTE"));
    SAME_TRIP_PROB = std::stod(params.at("SAME_TRIP_PROB"));

    ALPHA = std::stod(params.at("ALPHA"));
    BETA = std::stod(params.at("BETA"));
    EVAPORATION = std::stod(params.at("EVAPORATION"));
    EPSILON = std::stod(params.at("EPSILON"));
}

bool ACOAlgorithm::dominates(const Ant& a, const Ant& b)
{
    if (b.path.empty())
        return true;

    if (!b.isValid)
        return a.isValid;

    if (!a.isValid || a.arrivalTime > b.arrivalTime || a.travelTime > b.travelTime || a.waitingTime > b.waitingTime || a.transfers > b.transfers)
        return false;

    if (a.arrivalTime < b.arrivalTime || a.travelTime < b.travelTime || a.waitingTime < b.waitingTime || a.transfers < b.transfers)
        return true;

    return false;
}

std::vector<Trip*> ACOAlgorithm::buildTripSequence(const Ant& ant)
{
    std::vector<Trip*> trips;

    Trip* previous = nullptr;

    for (const auto& edge : ant.path)
    {
        auto* trip = edge.from->getTrip();

        if (trip != previous)
        {
            trips.push_back(trip);
            previous = trip;
        }
    }

    return trips;
}

double ACOAlgorithm::pathSimilarity(const Ant& a, const Ant& b)
{
    auto tripsA = buildTripSequence(a);
    auto tripsB = buildTripSequence(b);

    if (tripsA.empty() || tripsB.empty())
        return 0.0;

    int common = 0;

    for (auto* tripA : tripsA)
    {
        if (std::find(tripsB.begin(), tripsB.end(), tripA) != tripsB.end())
            common++;
    }

    return common * 1.0 /
        std::max(tripsA.size(), tripsB.size());
}

double ACOAlgorithm::getPheromone(const std::array<std::unordered_map<ConnectionTime, double, ConnectionTimeHash>, 5>& pheromones,
    Objective objective,
    const ConnectionTime& edge)
{
    auto it = pheromones[objective].find(edge);

    if (it == pheromones[objective].end())
        return 1.0;

    return it->second;
}

double ACOAlgorithm::heuristicValue(Objective objective,
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

    switch (objective)
    {
    case ARRIVAL:
    case TRAVEL:
        return exp(progress) * exp(-0.1 * (wait - MIN_TRANSFER_TIME));

    case WAITING:
        return exp(progress) * exp(-0.3 * (wait - MIN_TRANSFER_TIME));

    case TRANSFERS:
        return transfer ? 0.2 : 3.0;
    }

    return 1.0;
}

StopTime* ACOAlgorithm::chooseNextDeparture(
    const std::vector<StopTime*>& departureOptions,
    const Stop* end,
    std::chrono::minutes arrival,
    StopTime* previous,
    Objective objective,
    const std::array<std::unordered_map<ConnectionTime, double, ConnectionTimeHash>, 5>& pheromones)
{

    if (randomDouble(0.0, 1.0) < EPSILON)
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
        double pheromone = getPheromone(pheromones, objective, edge);
        double heuristic = heuristicValue(objective, dep, end, arrival, previous);

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

Ant ACOAlgorithm::buildAnt(
    const Network& network,
    const Stop* start,
    const Stop* end,
    std::chrono::minutes departureTime,
    const std::array<std::unordered_map<ConnectionTime, double, ConnectionTimeHash>, 5>& pheromones)
{
    Ant ant;

    auto objective = (Objective)randomInt(0, 3);
    ant.objective = objective;

    auto startOptions = network.getStopTimes(start, departureTime, nullptr, 0, MAX_DEPARTURES_PER_ROUTE);

    if (startOptions.empty())
        return ant;

    auto current = chooseNextDeparture(startOptions, end, departureTime, nullptr, objective, pheromones);
    auto next = current->getNextStopTime();

    if (!current || !next)
        return ant;

    std::unordered_set<const Stop*> visited;

    ant.path.push_back({ current, next });

    while (next->getStop() != end && ant.path.size() < MAX_PATH_LENGTH)
    {
        visited.insert(current->getStop());

        auto departures = network.getStopTimes(next->getStop(),
            next->getTime(),
            next->getTrip(),
            MIN_TRANSFER_TIME,
            MAX_DEPARTURES_PER_ROUTE);

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
            [&](const auto& elem)
            {
                return elem->getTrip() == current->getTrip();
            });

        if (sameTripIt != filtered.end() && randomDouble(0.0, 1.0) < SAME_TRIP_PROB)
            current = *sameTripIt;
        else
            current = chooseNextDeparture(filtered, end, next->getTime(), current, objective, pheromones);

        if (!current || !current->getNextStopTime())
            break;

        next = current->getNextStopTime();

        ant.path.push_back({ current, next });
    }

    return ant;
}

void ACOAlgorithm::evaluateAnt(Ant& ant,
    std::chrono::minutes departureTime,
    const Stop* end)
{
    if (ant.path.empty())
        return;

    ant.isValid = ant.path.back().to->getStop() == end;

    if (!ant.isValid)
    {
        double minDistance = geoDistance(ant.path.front().from->getStop(), end);

        for (const auto& edge : ant.path)
        {
            minDistance = std::min(minDistance,
                geoDistance(edge.to->getStop(), end));
        }

        ant.distanceToGoal = minDistance;

        return;
    }

    auto startTime = ant.path.front().from->getTime();
    auto endTime = ant.path.back().to->getTime();

    ant.arrivalTime = endTime;
    ant.travelTime = endTime - startTime;
    ant.waitingTime = std::chrono::minutes(0);

    ant.transfers = 0;

    Trip* previousTrip = nullptr;
    auto arrival = departureTime;

    for (const auto& edge : ant.path)
    {
        auto* trip = edge.from->getTrip();

        if (previousTrip && previousTrip != trip)
        {
            ant.transfers++;

            auto wait = edge.from->getTime() - arrival;

            if (wait.count() > 0)
                ant.waitingTime += wait;
        }

        previousTrip = trip;
        arrival = edge.to->getTime();
    }
}

void ACOAlgorithm::evaporate(
    std::array<std::unordered_map<ConnectionTime, double, ConnectionTimeHash>, 5>& pheromones)
{
    for (int obj = 0; obj < 5; obj++)
    {
        for (auto& p : pheromones[obj])
        {
            p.second *= (1.0 - EVAPORATION);

            p.second = std::max(0.0001, p.second);
        }
    }
}

void ACOAlgorithm::updateParetoArchive(std::vector<Ant>& archive, const Ant& candidate)
{
    if (candidate.path.empty())
        return;

    for (const auto& ant : archive)
    {
        if (dominates(ant, candidate))
            return;
    }

    for (auto it = archive.begin(); it != archive.end();)
    {
        if (dominates(candidate, *it))
            it = archive.erase(it);
        else
            ++it;
    }

    for (const auto& ant : archive)
    {
        if (pathSimilarity(candidate, ant) > 0.90)
            return;
    }

    archive.push_back(candidate);

    if (archive.size() > ANT_COUNT)
    {
        archive.erase(archive.begin() + randomInt(0, archive.size() - 1));
    }
}

void ACOAlgorithm::reinforce(
    const std::vector<Ant>& archive,
    std::array<std::unordered_map<ConnectionTime, double, ConnectionTimeHash>, 5>& pheromones,
    std::chrono::minutes departureReference)
{
    for (const auto& ant : archive)
    {
        double reward = 1.0;

        if (!ant.isValid)
        {
            reward += 1.0 / (ant.distanceToGoal + 0.1);
        }
        else
        {
            switch (ant.objective)
            {
            case ARRIVAL:
            {
                double arrival =
                    (ant.arrivalTime - departureReference).count();

                reward += 10000.0 / (arrival + 1.0);

                break;
            }

            case TRAVEL:
            {
                reward += 10000.0 /
                    (ant.travelTime.count() + 1.0);

                break;
            }

            case WAITING:
            {
                reward += 10000.0 /
                    (ant.waitingTime.count() + 1.0);

                break;
            }

            case TRANSFERS:
            {
                reward += 10000.0 /
                    (ant.transfers + 1.0);

                break;
            }
            }
        }

        for (const auto& edge : ant.path)
        {
            auto& tau = pheromones[ant.objective][edge];
            tau += reward * 0.01;
            tau = std::clamp(tau, 0.001, 100.0);
        }
    }
}

std::vector<Ant> ACOAlgorithm::selectBestRoutes(const std::vector<Ant>& archive)
{
    std::vector<Ant> result;

    const Ant* bestArrival = nullptr;
    const Ant* bestTravel = nullptr;
    const Ant* bestWaiting = nullptr;
    const Ant* bestTransfers = nullptr;

    for (const auto& ant : archive)
    {
        if (!ant.isValid)
            continue;

        if (!bestArrival || ant.arrivalTime < bestArrival->arrivalTime)
            bestArrival = &ant;

        if (!bestTravel || ant.travelTime < bestTravel->travelTime)
            bestTravel = &ant;

        if (!bestWaiting || ant.waitingTime < bestWaiting->waitingTime)
            bestWaiting = &ant;

        if (!bestTransfers || ant.transfers < bestTransfers->transfers)
            bestTransfers = &ant;
    }

    auto add = [&](const Ant* ant)
        {
            if (!ant)
                return;

            for (const auto& existing : result)
            {
                if (pathSimilarity(existing, *ant) > 0.90)
                    return;
            }

            result.push_back(*ant);
        };

    add(bestArrival);
    add(bestTravel);
    add(bestWaiting);
    add(bestTransfers);

    return result;
}

std::vector<Path> ACOAlgorithm::findPath(
    const Network& network,
    const Stop* start,
    const Stop* end,
    std::chrono::minutes departureTime)
{
    if (start == end)
        return { Path("Starting stop equals destination!") };

    auto startOptions =
        network.getStopTimes(start, departureTime, nullptr, 0, MAX_DEPARTURES_PER_ROUTE);

    if (startOptions.empty())
        return { Path("No departures from starting stop!") };

    std::array<std::unordered_map<ConnectionTime, double, ConnectionTimeHash>, 5> pheromones;

    std::vector<Ant> archive;
    std::vector<Ant> bestRoutes;

    for (int iteration = 0; iteration < ITERATIONS; iteration++)
    {
        std::vector<Ant> ants;

        for (int i = 0; i < ANT_COUNT; i++)
        {
            auto ant = buildAnt(network, start, end, departureTime, pheromones);
            evaluateAnt(ant, departureTime, end);
            ants.push_back(ant);
        }

        if (iteration == 50)
            int x = 9;

        evaporate(pheromones);

        for (const auto& ant : ants)
        {
            updateParetoArchive(archive, ant);
        }

        bestRoutes = selectBestRoutes(archive);
        reinforce(bestRoutes, pheromones, departureTime);
    }

    std::vector<Path> results;

    for (const auto& ant : bestRoutes)
    {
        if (!ant.isValid)
            continue;

        results.push_back(Path(
            ant.path,
            ant.arrivalTime,
            ant.travelTime,
            ant.waitingTime,
            ant.transfers));
    }

    std::sort(results.begin(), results.end(),
        [](const Path& a, const Path& b)
        {
            if (a.getArrivalTime() == b.getArrivalTime())
                return a.getTransfers() < b.getTransfers();

            return a.getArrivalTime() < b.getArrivalTime();
        });

    if (results.empty())
        results.push_back(Path("Journey from start to end was not found!"));

    return results;
}