#include "Network.h"
#include <random>

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

const Stop* Network::getRandomStop() const
{
    auto size = stops.size();
    auto it = stops.begin();
    std::advance(it, randomInt(0, size));
    return it->second.get();
}

std::vector<StopTime*> Network::getStopTimes(const Stop* stop, std::chrono::minutes minTime) const
{
    std::vector<StopTime*> result;
    std::set<std::string> routeIds;
    for (const auto& route : stopTimesIndex.at(stop))
    {
        if(route.second.size() > 0 && route.second[0]->getNextStopTime() == nullptr)
            continue;
        for (const auto& dep : route.second)
            if (dep->getTime() >= minTime && dep->getNextStopTime() != nullptr)
            {
                result.push_back(dep);
                break;
            }
    }
    return result;
}