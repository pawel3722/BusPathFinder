#pragma once
#include <memory>
#include <unordered_map>
#include "Stop.h"
#include "Connection.h"
#include "Route.h"
#include "Service.h"

class Network {
    // immutable maps
    std::unordered_map<int, std::unique_ptr<Stop>> stops;
    std::unordered_map<int, std::unique_ptr<Connection>> connections;
    std::unordered_map<int, std::unique_ptr<Route>> routes;
    std::unordered_map<int, std::unique_ptr<Service>> services;

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
};
