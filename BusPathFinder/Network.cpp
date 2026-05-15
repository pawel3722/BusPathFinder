#include "Network.h"
#include "Functions.h"

#define MIN_TRANSFER_DURATION 3
#define MAX_DEPARTURES_PER_ROUTE 3

const Stop* Network::getRandomStop() const
{
    auto size = stops.size();
    auto it = stops.begin();
    std::advance(it, randomInt(0, size));
    return it->second.get();
}

std::vector<StopTime*> Network::getStopTimes(const Stop* stop, std::chrono::minutes minTime, const Trip* trip) const
{
    std::vector<StopTime*> result;

    if(stopTimesIndex.find(stop) != stopTimesIndex.end())
        for (const auto& route : stopTimesIndex.at(stop))
        {
            if (route.second.empty() || !route.second[0]->getNextStopTime())
                continue;
        
            int added = 0;

            for (const auto& dep : route.second)
            {
                if (!dep || !dep->getNextStopTime() || dep->getTime() < minTime)
                    continue;

                bool isTransfer = trip && dep->getTrip() != trip;

                auto requiredTime = isTransfer
                    ? minTime + std::chrono::minutes(MIN_TRANSFER_DURATION)
                    : minTime;

                if (dep->getTime() < requiredTime)
                    continue;

                result.push_back(dep);
                added++;
                /*if (!isTransfer || added >= MAX_DEPARTURES_PER_ROUTE)
                    break;*/
            }
        }
    return result;
}

const StopTime* Network::getLaterDeparture(const StopTime* stopTime) const
{
    auto& st = stopTimesIndex.at(stopTime->getStop()).at(stopTime->getTrip()->getRouteId());
    auto it = std::find(st.begin(), st.end(), stopTime);
    for (; it != st.end(); ++it)
    {
        if ((*it)->getTrip() != stopTime->getTrip())
            break;
    }
    return it != st.end() ? *it : nullptr;
}