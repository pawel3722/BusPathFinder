#pragma once
#include "Stop.h"
#include "Service.h"
#include "StopTime.h"

struct PathNode
{
	const Stop* startStop;
	const Stop* endStop;
	const std::string service;
	std::chrono::minutes departureTime;
	std::chrono::minutes arrivalTime;
};

class Path
{
	std::vector<PathNode> nodes;
	std::chrono::minutes arrivalTime;
	std::chrono::minutes travelTime;
	std::chrono::minutes waitingTime;
	double cost;
	int transfers;
public:
	Path() {};
	Path(std::vector<ConnectionTime> v, std::chrono::minutes a, std::chrono::minutes t, std::chrono::minutes w, double c, int tr);
	void addNode(Stop* start, Stop* end, std::string service, std::chrono::minutes departureTime, std::chrono::minutes arrivalTime)
	{
		nodes.push_back({start, end, service, departureTime, arrivalTime});
	}
	int getTransfers() const { return transfers; }
	const std::vector<PathNode>& getNodes() const { return nodes; }
	friend std::ostream& operator<<(std::ostream& os, const Path& path);
};