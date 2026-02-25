#pragma once
#include "Stop.h"

class Connection
{
private:
	const int id;
	const Stop* from;
	const Stop* to;
	const int time;
public:
	Connection(int c_id, int c_time, Stop* s_from, Stop* s_to): id(c_id), time(c_time), from(s_from), to(s_to) {}
	const int getId() const { return id; }
	const int getTime() const { return time; }
	const Stop* getFrom() const { return from; }
	const Stop* getTo() const { return to; }
};