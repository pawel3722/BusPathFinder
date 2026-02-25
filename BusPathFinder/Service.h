#pragma once
#include <chrono>
#include "Route.h"

class Service
{
private:
	const int id;
	const std::chrono::minutes startTime;
	const Route* route;
public:
	Service(int s_id, std::chrono::minutes c_time, Route* s_route) : id(s_id), startTime(c_time), route(s_route) {}
	const int getId() const { return id; }
	const std::chrono::minutes getStartTime() const { return startTime; }
	const Route* getRoute() const { return route; }
};