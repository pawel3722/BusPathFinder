#include "NetworkLoader.h"
#include "Functions.h"

#include <fstream>
#include <regex>
#include "json.hpp"

using json = nlohmann::json;

Network NetworkLoader::load(
    const std::string& stopsFile,
    const std::string& tripsFile,
    const std::string& stopTimesFile)
{
    std::unordered_map<int, std::unique_ptr<Stop>> stops;
    std::unordered_set<std::unique_ptr<StopTime>> stopTimes;
    std::unordered_map<std::string, std::unique_ptr<Trip>> trips;

    std::map<int, Stop*> platformsAssignments;

    // =========================
    // STOPS
    // =========================

    {
        std::ifstream file(stopsFile);

        if (!file)
            throw std::runtime_error("Cannot open stops file");

        json j;
        file >> j;

        for (const auto& stopJson : j)
        {
            int id = stopJson["id"];
            int zone = stopJson.value("zone", 0);

            std::string name = stopJson["name"];
            double lat = stopJson["lat"];
            double lon = stopJson["lon"];

            std::vector<int> platformsIds =
                stopJson["platforms"].get<std::vector<int>>();

            stops[id] = std::make_unique<Stop>(
                id,
                zone,
                std::move(name),
                lat,
                lon
            );

            for (const auto& el : platformsIds)
                platformsAssignments[el] = stops[id].get();
        }
    }

    // =========================
    // TRIPS
    // =========================

    {
        std::ifstream file(tripsFile);

        if (!file)
            throw std::runtime_error("Cannot open trips file");

        json j;
        file >> j;

        for (const auto& tripJson : j["trips"])
        {
            std::string id = tripJson["id"];
            std::string line = tripJson["line"];
            std::string direction = tripJson["direction"];
            std::string routeId = tripJson["shape_id"];
            std::string jobId = "";

            std::regex r(R"(_([^_]+)$)");
            std::smatch match;
            if (std::regex_search(id, match, r)) {
                jobId = match[1];
            }


            trips[id] = std::make_unique<Trip>(
                id,
                line,
                direction,
                routeId,
                jobId
            );
        }
    }

    // =========================
    // STOP TIMES
    // =========================

    {
        std::ifstream file(stopTimesFile);

        if (!file)
            throw std::runtime_error("Cannot open stop_times file");

        json j;
        file >> j;

        for (const auto& stopTimeJson : j["stopTimes"])
        {
            std::string tripId = stopTimeJson["trip_id"];

            Trip* trip = trips.at(tripId).get();

            std::chrono::minutes time =
                parseTime(stopTimeJson["time"]);

            int stopId = stopTimeJson["stop_id"];

            Stop* stop = platformsAssignments.at(stopId);

            int index = stopTimeJson["index"];

            auto ptr = std::make_unique<StopTime>(
                stop,
                trip,
                time,
                index
            );

            trip->addStopTime(
                ptr.get()
            );

            auto res = stopTimes.insert(std::move(ptr));
        }
    }

    return Network(
        std::move(stops),
        std::move(stopTimes),
        std::move(trips)
    );
}