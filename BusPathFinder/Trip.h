#pragma once

class StopTime;

class Trip
{
	const std::string id;
	const std::string line;
	const std::string direction;
	const std::string routeId;
	const std::string jobId;
	std::vector<StopTime*> stopTimes {};

public:
	Trip(std::string t_id, std::string t_line, std::string t_direction, std::string t_rId, std::string t_jId)
		: id(std::move(t_id)), line(std::move(t_line)), direction(std::move(t_direction)), routeId(t_rId), jobId(t_jId)  {}
	
	std::string getId() const { return id; }
	std::string getLine() const { return line; }
	std::string getDirection() const { return direction; }
	std::string getInfo() const { return line + " " + direction; }
	std::string getRouteId() const { return routeId; }
	std::string getJobId() const { return jobId; }
	
	StopTime* getStopTime(int index) const { return index < stopTimes.size() ? stopTimes.at(index) : nullptr; }
	StopTime* getLastStopTime() const { return !stopTimes.empty() ? stopTimes.back() : nullptr; }

	const std::vector<StopTime*>& getStopTimes() const { return stopTimes; }	
	void addStopTime(StopTime* st){ stopTimes.push_back(st); }
};