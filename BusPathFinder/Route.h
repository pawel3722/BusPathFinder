#pragma once
#include "Connection.h"
#include <vector>

class Route
{
private:
	const int id;
	const std::string name;
	const std::vector<Connection*> connections;
public:
	Route(int r_id, std::string r_name, std::vector<Connection*> r_connections) : id(r_id), name(std::move(r_name)), connections(std::move(r_connections)) {}
	const int getId() const { return id; }
	const std::string getName() const { return name; }
	const std::vector<Connection*> getConnections() const { return connections; }
	bool isLastStop(const Stop* stop) const
	{
		return connections.back()->getTo() == stop;
	}
};