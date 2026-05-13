#pragma once
#include <chrono>

class StopTime;

class Trip
{
	const std::string id;
	const std::string line;
	const std::string direction;
	const std::string routeId;
	std::vector<StopTime*> stopTimes {};

public:
	Trip(std::string t_id, std::string t_line, std::string t_direction, std::string t_rId)
		: id(std::move(t_id)), line(std::move(t_line)), direction(std::move(t_direction)), routeId(t_rId)  {}
	
	std::string getId() const { return id; }
	std::string getLine() const { return line; }
	std::string getDirection() const { return direction; }
	std::string getInfo() const { return line + " " + direction; }
	std::string getRouteId() const { return routeId; }

	const std::vector<StopTime*>& getStopTimes() const { return stopTimes; }
	
	void addStopTime(StopTime* st)
	{
		stopTimes.push_back(st);
	}
	
	StopTime* getStopTime(int index) const
	{
		return index < stopTimes.size() ? stopTimes.at(index) : nullptr;
	}
};