#include "NetworkLoader.h"
#include <fstream>
#include "json.hpp"

using json = nlohmann::json;

Network NetworkLoader::load(const std::string& filename)
{
    std::unordered_map<int, std::unique_ptr<Stop>> stops;
    std::unordered_map<int, std::unique_ptr<Connection>> connections;
    std::unordered_map<int, std::unique_ptr<Route>> routes;
    std::unordered_map<int, std::unique_ptr<Service>> services;

    std::ifstream file(filename);
    if (!file)
        throw std::runtime_error("Cannot open file");

    json j;
    file >> j;

    // --- STOPS ---
    for (const auto& stopJson : j["stops"])
    {
        int id = stopJson["id"];
        int zone = stopJson["zone"];
        std::string name = stopJson["name"];

        stops[id] = std::make_unique<Stop>(id, zone, std::move(name));
    }

    // --- CONNECTIONS ---
    for (const auto& connectionJson : j["connections"])
    {
        int id = connectionJson["id"];
        int time = connectionJson["time"];
        Stop* from = stops[connectionJson["from"]].get();
        Stop* to = stops[connectionJson["to"]].get();

        connections[id] = std::make_unique<Connection>(id, time, from, to);
    }

    // --- ROUTES ---
    for (const auto& routeJson : j["routes"])
    {
        int id = routeJson["id"];
        std::string name = routeJson["name"];
        std::vector<Connection*> routeConnections;
        std::vector<int> connectionIds = routeJson["connections"].get<std::vector<int>>();
        for (const auto& el : connectionIds)
        {
            routeConnections.push_back(connections[el].get());
        }
        
        routes[id] = std::make_unique<Route>(id, std::move(name), std::move(routeConnections));
    }

    // --- SERVICES ---
    for (const auto& serviceJson : j["services"])
    {
        int id = serviceJson["id"];
        std::chrono::minutes time = parseTime(serviceJson["startTime"]);
        Route* route = routes[serviceJson["route"]].get();
        services[id] = std::make_unique<Service>(id, time, route);
    }

    return Network(
        std::move(stops),
        std::move(connections),
        std::move(routes),
        std::move(services)
    );
}

std::chrono::minutes NetworkLoader::parseTime(const std::string& str)
{
    if (str.size() != 5 || str[2] != ':')
        throw std::invalid_argument("Invalid time format");

    int hour = std::stoi(str.substr(0, 2));
    int minute = std::stoi(str.substr(3, 2));

    if (hour < 0 || hour > 23 || minute < 0 || minute > 59)
        throw std::out_of_range("Time out of range");

    return std::chrono::hours(hour) + std::chrono::minutes(minute);
}