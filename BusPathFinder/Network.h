#pragma once
#include <memory>
#include <unordered_map>
#include "Stop.h"
#include "Trip.h"
#include "StopTime.h"
#include <set>
#include <algorithm>

class Network {
    // immutable maps
    std::unordered_map<int, std::unique_ptr<Stop>> stops;
    std::unordered_set<std::unique_ptr<StopTime>> stopTimes;
    std::unordered_map<std::string,std::unique_ptr<Trip>> trips;

    std::unordered_map<const Stop*, std::unordered_map<std::string, std::vector<StopTime*>>> stopTimesIndex;

public:
    Network(
        std::unordered_map<int, std::unique_ptr<Stop>> s,
        std::unordered_set<std::unique_ptr<StopTime>> st,
        std::unordered_map<std::string, std::unique_ptr<Trip>> t
    )
        : stops(std::move(s)),
       stopTimes(std::move(st)),
       trips(std::move(t))
    {
        for (const auto& el : stopTimes)
        {
            auto ptr = el.get();
            stopTimesIndex[ptr->getStop()][ptr->getTrip()->getRouteId()].push_back(ptr);
        }


        for (auto& el : stopTimesIndex)
        {
            for (auto& pos : el.second)
            {
                std::sort(pos.second.begin(), pos.second.end(),
                    [](const StopTime* a,
                        const StopTime* b)
                    {
                        return a->getTime() < b->getTime();
                    });
            }
        }
    }

    const Stop* getStop(int id) const {
        auto it = stops.find(id);
        return it != stops.end() ? it->second.get() : nullptr;
    }

    const Stop* getRandomStop() const;

    std::vector<StopTime*> getStopTimes(const Stop* stop, std::chrono::minutes minTime, const Trip* trip = nullptr) const;
    const StopTime* getLaterDeparture(const StopTime* stopTime) const;
};
