#pragma once
#include <memory>
#include <unordered_map>
#include "Stop.h"
#include "Connection.h"
#include "Route.h"
#include "Service.h"
#include "Trip.h"
#include "StopTime.h"
#include <set>
#include <algorithm>

class Network {
    // immutable maps
    std::unordered_map<int, std::unique_ptr<Stop>> stops;
    std::unordered_map<int, std::unique_ptr<Connection>> connections;
    std::unordered_map<int, std::unique_ptr<Route>> routes;
    std::unordered_map<int, std::unique_ptr<Service>> services;


    std::unordered_map<const Stop*,std::vector<std::unique_ptr<StopTime>> > stopTimes;
    std::set<std::unique_ptr<Trip>> trips;

public:
    Network(
        std::unordered_map<int, std::unique_ptr<Stop>> s,
        std::unordered_map<int, std::unique_ptr<Connection>> c,
        std::unordered_map<int, std::unique_ptr<Route>> r,
        std::unordered_map<int, std::unique_ptr<Service>> svc
    )
        : stops(std::move(s)),
        connections(std::move(c)),
        routes(std::move(r)),
        services(std::move(svc))
    {
        for (const auto& svc : services)
        {
            auto time = svc.second->getStartTime();
            auto conns = svc.second->getRoute()->getConnections();
            std::vector<StopTime*> localStopTimes;

            for (int i = 0; i < conns.size(); i++)
            {
                auto conn = conns[i];

                if (i == 0)
                {
                    auto stop = conn->getFrom();
                    auto ptr = std::make_unique<StopTime>(stop, time, i);
                    localStopTimes.push_back(ptr.get());
                    stopTimes[stop].push_back(std::move(ptr));
                }
                auto stop = conn->getTo();
                time += std::chrono::minutes(conn->getTime());
                auto ptr = std::make_unique<StopTime>(stop, time, i + 1);;
                localStopTimes.push_back(ptr.get());
                stopTimes[stop].push_back(std::move(ptr));
                
            }
            auto trp = std::make_unique<Trip>(svc.second.get(), localStopTimes);
            for (auto& st : localStopTimes)
            {
                st->setTrip(trp.get(), svc.second->getRoute()->getName());
            }
            trips.insert(std::move(trp));
        }

        for (auto& el : stopTimes)
        {
            std::sort(el.second.begin(), el.second.end(),
                [](const std::unique_ptr<StopTime>& a,
                    const std::unique_ptr<StopTime>& b)
                {
                    return a->getTime() < b->getTime();
                });
        }
    }

    const Stop* getStop(int id) const {
        auto it = stops.find(id);
        return it != stops.end() ? it->second.get() : nullptr;
    }

    const Connection* getConnection(int id) const {
        auto it = connections.find(id);
        return it != connections.end() ? it->second.get() : nullptr;
    }

    const Route* getRoute(int id) const {
        auto it = routes.find(id);
        return it != routes.end() ? it->second.get() : nullptr;
    }

    const Service* getService(int id) const {
        auto it = services.find(id);
        return it != services.end() ? it->second.get() : nullptr;
    }

    std::vector<StopTime*> getStopTimes(const Stop* stop, std::chrono::minutes minTime) const;

    
};
