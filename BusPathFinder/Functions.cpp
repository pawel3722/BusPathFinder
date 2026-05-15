#include "Functions.h"
#include "Stop.h"

#define M_PI 3.14159265

std::chrono::minutes randomTime()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_int_distribution<int> dist(0, 23 * 60 + 59);

    return std::chrono::minutes(dist(gen));
}

int randomInt(int a, int b)
{
    if (a == b)
        return a;
    if (a > b)
        std::swap(a, b);
    std::uniform_int_distribution<int> dist(a, b);
    return dist(rng);
}

double randomDouble(double a, double b)
{
    std::uniform_real_distribution<double> dist(a, b);
    return dist(rng);
}

double haversine(double lat1, double lon1, double lat2, double lon2)
{
    // distance between latitudes
    // and longitudes
    double dLat = (lat2 - lat1) *
        M_PI / 180.0;
    double dLon = (lon2 - lon1) *
        M_PI / 180.0;

    // convert to radians
    lat1 = (lat1)*M_PI / 180.0;
    lat2 = (lat2)*M_PI / 180.0;

    // apply formulae
    double a = pow(sin(dLat / 2), 2) +
        pow(sin(dLon / 2), 2) *
        cos(lat1) * cos(lat2);
    double rad = 6371;
    double c = 2 * asin(sqrt(a));
    return rad * c;
}

std::string formatTime(std::chrono::minutes mins)
{
    int total = (int)mins.count();

    int hours = total / 60;
    int minutes = total % 60;

    std::ostringstream oss;

    oss << std::setw(2) << std::setfill('0') << hours
        << ":"
        << std::setw(2) << std::setfill('0') << minutes;

    return oss.str();
}

std::chrono::minutes parseTime(const std::string& str)
{
    // obsługa HH:MM lub HH:MM:SS

    if (str.size() != 5 && str.size() != 8)
        throw std::invalid_argument("Invalid time format");

    int hour = std::stoi(str.substr(0, 2));
    int minute = std::stoi(str.substr(3, 2));

    // GTFS pozwala na godziny > 23
    // np. 26:30:00

    if (hour < 0 || minute < 0 || minute > 59)
        throw std::out_of_range("Time out of range");

    return std::chrono::hours(hour)
        + std::chrono::minutes(minute);
}

double geoDistance(const Stop* a, const Stop* b)
{
    return haversine(a->getLat(), a->getLon(), b->getLat(), b->getLon());
}
