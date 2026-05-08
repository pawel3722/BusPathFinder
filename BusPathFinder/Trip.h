#pragma once
#include "Stop.h"
#include <chrono>
#include "Service.h"
#include "StopTime.h"
class Trip
{
	const Service* service;
	const std::vector<StopTime*> stopTimes;
public:
	Trip(const Service* s, std::vector<StopTime*> st) : service(s), stopTimes(std::move(st)) {}
	const Service* getService() const { return service; }
	const std::vector<StopTime*>& getStopTimes() const { return stopTimes; }
	const StopTime* getStopTime(int index) const
	{
		return stopTimes.at(index);
	}
};