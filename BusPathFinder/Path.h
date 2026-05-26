#pragma once
#include "Stop.h"
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
	std::chrono::minutes arrivalTime = std::chrono::minutes(0);
	std::chrono::minutes travelTime = std::chrono::minutes(0);
	std::chrono::minutes waitingTime = std::chrono::minutes(0);
	double cost = 0.0;
	int transfers = 0;
	std::string message = "";
public:
	Path() {};
	Path(std::string m) { message = std::move(m); };
	Path(std::vector<ConnectionTime> v, std::chrono::minutes a, std::chrono::minutes t, std::chrono::minutes w, double c, int tr);
	void addNode(Stop* start, Stop* end, std::string service, std::chrono::minutes departureTime, std::chrono::minutes arrivalTime)
	{
		nodes.push_back({start, end, service, departureTime, arrivalTime});
	}

	int getTransfers() const { return transfers; }
	std::chrono::minutes getArrivalTime() const { return arrivalTime; }
	std::chrono::minutes getTravelTime() const { return travelTime; }
	std::chrono::minutes getWaitingTime() const { return waitingTime; }

	const std::vector<PathNode>& getNodes() const { return nodes; }
	friend std::ostream& operator<<(std::ostream& os, const Path& path);

	bool operator==(const Path& other) const;
};