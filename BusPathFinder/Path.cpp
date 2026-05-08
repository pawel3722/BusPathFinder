#include <iostream>
#include "Path.h"

std::ostream& operator<<(std::ostream& os, const Path& path)
{
	os << "================PATH================"<< std::endl;
	for (const auto& node : path.nodes)
	{
		os  << "Service: " << node.service->getRoute()->getName() << std::endl
		    << "\tFrom: " << node.startStop->getName() 
			<< " To: " << node.endStop->getName() << std::endl
		    << "\tDeparture: " << node.departureTime 
		    << " Arrival: " << node.arrivalTime << std::endl;
	}
	return os;
}

Path::Path(std::vector<StopTime> v)
{
	Trip* currentTrip = nullptr;
	for (int i = 0; i < v.size() - 1; i++)
	{
		currentTrip = v[i].getTrip();
	}
}
