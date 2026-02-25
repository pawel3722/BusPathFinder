#pragma once
#include <string>

class Stop
{
private:
	const int id;
	const int zone;
	const std::string name;
public:
	Stop(int s_id, int s_zone, std::string s_name) : id(s_id), zone(s_zone), name(std::move(s_name)) {}
	const int getId() const { return id; }
	const int getZone() const { return zone; }
	const std::string getName() const { return name; }
};