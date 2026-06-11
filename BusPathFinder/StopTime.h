#pragma once
#include "Stop.h"
#include "Trip.h"
#include <chrono>

struct ConnectionTime
{
	const StopTime* from;
	const StopTime* to;

	bool operator== (const ConnectionTime& other) const
	{
		return from == other.from && to == other.to;
	}
};

struct ConnectionTimeHash
{
	size_t operator()(const ConnectionTime& c) const
	{
		size_t h1 = std::hash<const void*>()(c.from);
		size_t h2 = std::hash<const void*>()(c.to);

		return h1 ^ (h2 << 1);
	}
};

class StopTime
{
	const Stop* stop;
	const std::chrono::minutes time;
	Trip* trip;
	size_t indexInRoute;
public:
	StopTime(const Stop* s, Trip* tr, std::chrono::minutes t, size_t i) : stop(s), trip(tr), time(t), indexInRoute(i) {}
	const Stop* getStop() const { return stop; }
	const std::chrono::minutes getTime() const { return time; }
	Trip* getTrip() const { return trip; }
	const size_t getIndexInRoute() const { return indexInRoute; }
	StopTime* getNextStopTime() const { return trip->getStopTime(indexInRoute + 1); }
};