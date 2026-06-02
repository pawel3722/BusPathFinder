#include "NetworkLoaderGdansk.h"
#include "Functions.h"

#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

// =========================
// CSV helpers
// =========================

static std::vector<std::string> parseCsvLine(const std::string& line)
{
    std::vector<std::string> result;
    std::string current;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i)
    {
        char c = line[i];

        if (c == '"')
        {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"')
            {
                current += '"';
                ++i;
            }
            else
            {
                inQuotes = !inQuotes;
            }
        }
        else if (c == ',' && !inQuotes)
        {
            result.push_back(current);
            current.clear();
        }
        else
        {
            current += c;
        }
    }

    result.push_back(current);
    return result;
}

static std::unordered_map<std::string, int> makeHeaderIndex(
    const std::vector<std::string>& header
)
{
    std::unordered_map<std::string, int> result;

    for (int i = 0; i < static_cast<int>(header.size()); ++i)
        result[header[i]] = i;

    return result;
}

static std::string getRequired(
    const std::vector<std::string>& row,
    const std::unordered_map<std::string, int>& header,
    const std::string& column
)
{
    auto it = header.find(column);

    if (it == header.end())
        throw std::runtime_error("Missing required GTFS column: " + column);

    int index = it->second;

    if (index < 0 || index >= static_cast<int>(row.size()))
        return "";

    return row[index];
}

static std::string getOptional(
    const std::vector<std::string>& row,
    const std::unordered_map<std::string, int>& header,
    const std::string& column,
    const std::string& defaultValue = ""
)
{
    auto it = header.find(column);

    if (it == header.end())
        return defaultValue;

    int index = it->second;

    if (index < 0 || index >= static_cast<int>(row.size()))
        return defaultValue;

    return row[index];
}

static bool endsWith(const std::string& str, const std::string& suffix)
{
    if (suffix.size() > str.size())
        return false;

    return std::equal(
        suffix.rbegin(),
        suffix.rend(),
        str.rbegin()
    );
}

static std::string trim(const std::string& s)
{
    size_t start = 0;

    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
        ++start;

    size_t end = s.size();

    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
        --end;

    return s.substr(start, end - start);
}

static std::string baseStopName(const std::string& name)
{
    std::string s = trim(name);

    size_t pos = s.find_last_of(' ');

    if (pos == std::string::npos)
        return s;

    std::string last = s.substr(pos + 1);

    if (last.empty())
        return s;

    for (char c : last)
    {
        if (!std::isdigit(static_cast<unsigned char>(c)))
            return s;
    }

    return trim(s.substr(0, pos));
}

// =========================
// Loader
// =========================

Network NetworkLoaderGdansk::load(
    const std::string& gtfsDirectory,
    const std::string& targetDate
)
{
    std::unordered_map<int, std::unique_ptr<Stop>> stops;
    std::unordered_set<std::unique_ptr<StopTime>> stopTimes;
    std::unordered_map<std::string, std::unique_ptr<Trip>> trips;

    std::map<int, Stop*> platformsAssignments;

    fs::path dir(gtfsDirectory);

    fs::path stopsFile = dir / "stops.txt";
    fs::path routesFile = dir / "routes.txt";
    fs::path tripsFile = dir / "trips.txt";
    fs::path stopTimesFile = dir / "stop_times.txt";

    // =========================
    // ROUTES
    // route_id -> route_short_name
    // =========================

    std::unordered_map<std::string, std::string> routesMap;

    {
        std::ifstream file(routesFile);

        if (!file)
            throw std::runtime_error("Cannot open routes.txt");

        std::string line;

        if (!std::getline(file, line))
            throw std::runtime_error("routes.txt is empty");

        auto header = makeHeaderIndex(parseCsvLine(line));

        while (std::getline(file, line))
        {
            if (line.empty())
                continue;

            auto row = parseCsvLine(line);

            std::string routeId = getRequired(row, header, "route_id");
            std::string routeShortName =
                getOptional(row, header, "route_short_name", routeId);

            if (routeShortName.empty())
                routeShortName = routeId;

            routesMap[routeId] = routeShortName;
        }
    }

    // =========================
    // STOPS
    // grupowanie platform po nazwie bazowej
    // =========================

    struct StopGroup
    {
        int id = -1;
        int zone = 0;
        std::string name;
        double latSum = 0.0;
        double lonSum = 0.0;
        int count = 0;
        std::vector<int> platformIds;
    };

    std::unordered_map<std::string, StopGroup> groupedStops;

    {
        std::ifstream file(stopsFile);

        if (!file)
            throw std::runtime_error("Cannot open stops.txt");

        std::string line;

        if (!std::getline(file, line))
            throw std::runtime_error("stops.txt is empty");

        auto header = makeHeaderIndex(parseCsvLine(line));

        while (std::getline(file, line))
        {
            if (line.empty())
                continue;

            auto row = parseCsvLine(line);

            int platformId = std::stoi(getRequired(row, header, "stop_id"));

            std::string stopName = getRequired(row, header, "stop_name");
            std::string groupedName = baseStopName(stopName);

            double lat = std::stod(getRequired(row, header, "stop_lat"));
            double lon = std::stod(getRequired(row, header, "stop_lon"));

            int zone = 0;
            std::string zoneStr = getOptional(row, header, "zone_id", "0");

            if (!zoneStr.empty())
            {
                try
                {
                    zone = std::stoi(zoneStr);
                }
                catch (...)
                {
                    zone = 0;
                }
            }

            auto& group = groupedStops[groupedName];

            if (group.count == 0)
            {
                group.id = platformId;
                group.zone = zone;
                group.name = groupedName;
            }
            else
            {
                group.id = std::min(group.id, platformId);
            }

            group.latSum += lat;
            group.lonSum += lon;
            group.count++;
            group.platformIds.push_back(platformId);
        }
    }

    for (auto& [name, group] : groupedStops)
    {
        double lat = group.latSum / group.count;
        double lon = group.lonSum / group.count;

        stops[group.id] = std::make_unique<Stop>(
            group.id,
            group.zone,
            group.name,
            lat,
            lon
        );

        Stop* stopPtr = stops[group.id].get();

        for (int platformId : group.platformIds)
            platformsAssignments[platformId] = stopPtr;
    }

    // =========================
    // TRIPS
    // service_id kończące się na targetDate
    // =========================

    {
        std::ifstream file(tripsFile);

        if (!file)
            throw std::runtime_error("Cannot open trips.txt");

        std::string line;

        if (!std::getline(file, line))
            throw std::runtime_error("trips.txt is empty");

        auto header = makeHeaderIndex(parseCsvLine(line));

        std::regex r(R"(_([^_]+)$)");

        while (std::getline(file, line))
        {
            if (line.empty())
                continue;

            auto row = parseCsvLine(line);

            std::string serviceId = getRequired(row, header, "service_id");

            if (!endsWith(serviceId, targetDate))
                continue;

            std::string id = getRequired(row, header, "trip_id");
            std::string routeIdRaw = getRequired(row, header, "route_id");

            std::string lineName = routeIdRaw;

            auto routeIt = routesMap.find(routeIdRaw);
            if (routeIt != routesMap.end())
                lineName = routeIt->second;

            std::string direction = getOptional(row, header, "trip_headsign", "");
            std::string routeId = getOptional(row, header, "shape_id", "");

            std::string jobId = "";

            std::smatch match;
            if (std::regex_search(id, match, r))
                jobId = match[1];

            trips[id] = std::make_unique<Trip>(
                id,
                lineName,
                direction,
                routeId,
                jobId
            );
        }
    }

    // =========================
    // STOP TIMES
    // tylko trip_id istniejące w trips
    // =========================

    {
        std::ifstream file(stopTimesFile);

        if (!file)
            throw std::runtime_error("Cannot open stop_times.txt");

        std::string line;

        if (!std::getline(file, line))
            throw std::runtime_error("stop_times.txt is empty");

        auto header = makeHeaderIndex(parseCsvLine(line));

        while (std::getline(file, line))
        {
            if (line.empty())
                continue;

            auto row = parseCsvLine(line);

            std::string tripId = getRequired(row, header, "trip_id");

            auto tripIt = trips.find(tripId);

            if (tripIt == trips.end())
                continue;

            Trip* trip = tripIt->second.get();

            std::chrono::minutes time =
                parseTime(getRequired(row, header, "arrival_time"));

            int platformId = std::stoi(getRequired(row, header, "stop_id"));

            auto stopIt = platformsAssignments.find(platformId);

            if (stopIt == platformsAssignments.end())
                continue;

            Stop* stop = stopIt->second;

            int index = std::stoi(getRequired(row, header, "stop_sequence"));

            auto ptr = std::make_unique<StopTime>(
                stop,
                trip,
                time,
                index
            );

            trip->addStopTime(ptr.get());

            stopTimes.insert(std::move(ptr));
        }
    }

    return Network(
        std::move(stops),
        std::move(stopTimes),
        std::move(trips)
    );
}