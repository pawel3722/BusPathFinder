#pragma once
#include <string>
#include <unordered_set>

class Stop
{
private:
	const int id;
	const int zone;
	const std::string name;
	const double lat;
	const double lon;
public:
	Stop(int s_id, int s_zone, std::string s_name, double s_lat, double s_lon)
		: id(s_id), zone(s_zone), name(std::move(s_name)), lat(s_lat), lon(s_lon) {}
	const int getId() const { return id; }
	const int getZone() const { return zone; }
	const std::string getName() const { return name; }
	const double getLat() const { return lat; }
	const double getLon() const { return lon; }
};