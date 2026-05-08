#pragma once
#include "Stop.h"
#include "Service.h"
#include "StopTime.h"

struct PathNode
{
	const Stop* startStop;
	const Stop* endStop;
	const Service* service;
	const int departureTime;
	const int arrivalTime;	
};

class Path
{
	std::vector<PathNode> nodes;
public:
	Path(std::vector<StopTime> v);
	void addNode(const Stop* start, const Stop* end, const Service* service, int departureTime, int arrivalTime)
	{
		nodes.push_back({start, end, service, departureTime, arrivalTime});
	}
	const std::vector<PathNode>& getNodes() const { return nodes; }
	friend std::ostream& operator<<(std::ostream& os, const Path& path);
};