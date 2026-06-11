#pragma once
#include "Network.h"
#include "Path.h"

enum Objective
{
    ARRIVAL,
    TRAVEL,
    WAITING,
    TRANSFERS,
};

class IAlgorithm
{
public:
	virtual std::vector<Path> findPath(const Network & network, const Stop * start, const Stop * end, std::chrono::minutes departureTime) = 0;
};