#pragma once
#include "Stop.h"
#include "Service.h"
#include "StopTime.h"

struct PathNode
{
	const Stop* startStop;
	const Stop* endStop;
	const Service* service;
	std::chrono::minutes departureTime;
	std::chrono::minutes arrivalTime;
};

class Path
{
	std::vector<PathNode> nodes;
public:
	Path() {};
	Path(std::vector<const StopTime*> v);
	void addNode(Stop* start, Stop* end, Service* service, std::chrono::minutes departureTime, std::chrono::minutes arrivalTime)
	{
		nodes.push_back({start, end, service, departureTime, arrivalTime});
	}
	const std::vector<PathNode>& getNodes() const { return nodes; }
	friend std::ostream& operator<<(std::ostream& os, const Path& path);
};