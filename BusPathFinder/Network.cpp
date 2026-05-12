#include "Network.h"

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