#include <iostream>
#include <chrono>

#include "Path.h"
#include "Functions.h"


std::ostream& operator<<(std::ostream& os, const Path& path)
{
	if (!path.message.empty())
	{
		os << path.message << std::endl;
		return os;
	}

	os << "================PATH================"<< std::endl;
	os << "Arrival time: " << formatTime(path.arrivalTime) << std::endl;
	os << "Travel time:  " << formatTime(path.travelTime) << std::endl;
	os << "Waiting time: " << formatTime(path.waitingTime) << std::endl;
	os << "Cost:         " << path.cost << std::endl;
	os << "Transfers:    " << path.transfers << std::endl;
	os << "- - - - - - - - - - - - - - - - - - "<< std::endl;
	for (const auto& node : path.nodes)
	{
		os  << "Service: " << node.service << std::endl
		    << "\tFrom: " << node.startStop->getName() 
			<< " To: " << node.endStop->getName() << std::endl
		    << "\tDeparture: " << formatTime(std::chrono::minutes(node.departureTime))
		    << " Arrival: " << formatTime(std::chrono::minutes(node.arrivalTime)) << std::endl;
	}
	return os;
}

Path::Path(std::vector<ConnectionTime> v, std::chrono::minutes a, std::chrono::minutes t, std::chrono::minutes w, double c, int tr)
{
	arrivalTime = a;
	travelTime = t;
	waitingTime = w;
	cost = c;
	transfers = tr;

	Trip* currentTrip = nullptr;
	auto nodeIndex = -1;
	for (int i = 0; i < v.size(); i++)
	{
		if (v[i].to->getTrip() != currentTrip)
		{
			currentTrip = v[i].to->getTrip();
			nodes.push_back({ v[i].from->getStop(), nullptr, v[i].from->getTrip()->getInfo(), v[i].from->getTime(), std::chrono::minutes(0)});
			if (nodeIndex >= 0)
			{
				nodes[nodeIndex].endStop = v[i-1].to->getStop();
				nodes[nodeIndex].arrivalTime = v[i-1].to->getTime();
			}
			nodeIndex++;
		}
	}
	if (nodeIndex >= 0)
	{
		nodes[nodeIndex].endStop = v.back().to->getStop();
		nodes[nodeIndex].arrivalTime = v.back().to->getTime();
	}
}
