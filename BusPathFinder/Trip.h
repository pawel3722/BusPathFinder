#pragma once
#include "Stop.h"
#include <chrono>
#include "Service.h"

class StopTime;

class Trip
{
	const Service* service;
	std::vector<StopTime*> stopTimes;
public:
	Trip(const Service* s) : service(s) {}
	void addStopTime(StopTime* st)
	{
		stopTimes.push_back(st);
	}
	const Service* getService() const { return service; }
	const std::vector<StopTime*>& getStopTimes() const { return stopTimes; }
	StopTime* getStopTime(int index) const
	{
		return stopTimes.at(index);
	}
};