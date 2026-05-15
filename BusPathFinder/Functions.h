#pragma once
#include <chrono>
#include <random>
#include <iomanip>
#include <sstream>
#include "Stop.h"

static std::mt19937 rng(std::random_device{}());

std::chrono::minutes randomTime();

int randomInt(int a, int b);

double randomDouble(double a, double b);

double haversine(double lat1, double lon1, double lat2, double lon2);

std::string formatTime(std::chrono::minutes mins);

std::chrono::minutes parseTime(const std::string& str);

double geoDistance(const Stop* a, const Stop* b);


