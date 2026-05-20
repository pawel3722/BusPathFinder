#include "Network.h"
#include "Functions.h"

const Stop* Network::getRandomStop() const
{
    auto size = stops.size();
    auto it = stops.begin();
    std::advance(it, randomInt(0, size));
    return it->second.get();
}

std::vector<StopTime*> Network::getStopTimes(const Stop* stop, std::chrono::minutes minTime, const Trip* trip, int minTransferDuration, int maxDeparturesPerRoute) const
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

                bool isTransfer = trip && dep->getTrip()->getJobId() != trip->getJobId();

                auto requiredTime = isTransfer
                    ? minTime + std::chrono::minutes(minTransferDuration)
                    : minTime;

                if (dep->getTime() < requiredTime)
                    continue;

                result.push_back(dep);
                added++;
                if ((!isTransfer && trip) || added >= maxDeparturesPerRoute)
                    break;
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

const StopTime* Network::getEarlierDeparture(const StopTime* stopTime) const
{
    auto& st = stopTimesIndex.at(stopTime->getStop()).at(stopTime->getTrip()->getRouteId());
    auto it = std::find(st.rbegin(), st.rend(), stopTime);
    for (; it != st.rend(); ++it)
    {
        if ((*it)->getTrip() != stopTime->getTrip())
            break;
    }
    return it != st.rend() ? *it : nullptr;
}

const StopTime* Network::getCommonStop(Trip* t1, Trip* t2, const StopTime* start, const StopTime* end) const
{
    auto& st1 = t1->getStopTimes();
    auto& st2 = t2->getStopTimes();

    const StopTime* stop = nullptr;

    for (auto it = std::find(st1.begin(), st1.end(), start); it != st1.end(); it++)
    {
        for (const auto& el : st2)
        {
            if ((*it)->getStop() == el->getStop())
                stop = el;
            if (el == end)
                break;
        }
    }

    return stop;
}