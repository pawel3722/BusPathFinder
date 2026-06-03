#pragma once
#include "Path.h"
#include <chrono>

struct Result
{
	std::vector<Path> paths;
	std::chrono::minutes bestArrivalTime = std::chrono::minutes::max();
	std::chrono::minutes bestTravelTime = std::chrono::minutes::max();
	std::chrono::minutes bestWaitingTime = std::chrono::minutes::max();
	int bestTransfers = std::numeric_limits<int>::max();
	std::chrono::milliseconds computationTime = std::chrono::milliseconds::max();
	bool isValid = false;

	Result(std::vector<Path> p, std::chrono::milliseconds compTime): computationTime(compTime)
	{
		for (const auto& path : p)
		{
			if (!path.isValid())
				continue;
			if (path.getArrivalTime() < bestArrivalTime)
				bestArrivalTime = path.getArrivalTime();
			if (path.getTravelTime() < bestTravelTime)
				bestTravelTime = path.getTravelTime();
			if (path.getWaitingTime() < bestWaitingTime)
				bestWaitingTime = path.getWaitingTime();
			if (path.getTransfers() < bestTransfers)
				bestTransfers = path.getTransfers();
			paths.push_back(path);
			isValid = true;
		}
	}
};
