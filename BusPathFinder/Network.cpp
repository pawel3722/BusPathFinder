#include "Network.h"

std::vector<StopTime*> Network::getStopTimes(const Stop* stop, std::chrono::minutes minTime) const
{
    std::vector<StopTime*> result;
    std::set<int> routeIds;
    for (const auto& el : stopTimes.at(stop))
    {
        auto route = el->getTrip()->getService()->getRoute();
        if (el->getTime() >= minTime 
            && routeIds.find(route->getId()) == routeIds.end() 
            && !route->isLastStop(stop))
        {
            result.push_back(el.get());
            routeIds.insert(route->getId());
        }
    }
    return result;
}