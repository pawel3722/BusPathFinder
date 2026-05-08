#pragma once
#include "Stop.h"
#include "Trip.h"
#include <chrono>

class StopTime
{
	const Stop* stop;
	std::string direction = "";
	const std::chrono::minutes time;
	Trip* trip;
	size_t indexInRoute;
public:
	StopTime(const Stop* s, Trip* tr, std::chrono::minutes t, size_t i) : stop(s), trip(tr), time(t), indexInRoute(i) {}
	const Stop* getStop() const { return stop; }
	const std::chrono::minutes getTime() const { return time; }
	Trip* getTrip() const { return trip; }
	const size_t getIndexInRoute() const { return indexInRoute; }
};