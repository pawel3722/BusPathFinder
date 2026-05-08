#pragma once
#include "Stop.h"
#include <chrono>

class Trip;

class StopTime
{
	const Stop* stop;
	std::string direction = "";
	const std::chrono::minutes time;
	Trip* trip = nullptr;
	size_t indexInRoute;
public:
	StopTime(const Stop* s, std::chrono::minutes t, size_t i) : stop(s), time(t), indexInRoute(i) {}
	const Stop* getStop() const { return stop; }
	const std::chrono::minutes getTime() const { return time; }
	void setTrip(Trip* t, std::string dir) { trip = t; direction = dir; }
	Trip* getTrip() const { return trip; }
	const size_t getIndexInRoute() const { return indexInRoute; }
};