#include "GeneticAlgorithm.h"

#include <random>
#include <algorithm>

static std::mt19937 rng(std::random_device{}());

static int randomInt(int a, int b)
{
    std::uniform_int_distribution<int> dist(a, b);
    return dist(rng);
}

static int fitness(const std::vector<const StopTime*>& path,
    const Stop* end)
{
    if (path.empty()) return INT_MAX;

    const StopTime* last = path.back();

    if (last->getStop() != end)
        return INT_MAX / 2 - path.size();

    return (int)path.back()->getTime().count();
}

static void mutate(std::vector<const StopTime*>& path,
    const std::vector<StopTime*>& options)
{
    if (path.empty() || options.empty()) return;

    int idx = randomInt(0, path.size() - 1);
    int r = randomInt(0, options.size() - 1);

    path[idx] = options[r];
}

static std::vector<const StopTime*> crossover(
    const std::vector<const StopTime*>& a,
    const std::vector<const StopTime*>& b)
{
    std::vector<const StopTime*> child;

    if (a.empty() || b.empty()) return child;

    int cutA = randomInt(0, a.size() - 1);
    int cutB = randomInt(0, b.size() - 1);

    child.insert(child.end(), a.begin(), a.begin() + cutA);
    child.insert(child.end(), b.begin() + cutB, b.end());

    return child;
}


Path GeneticAlgorithm::findPath(const Network& network, const Stop* start, const Stop* end, std::chrono::minutes departureTime)
{
    auto startOptions = network.getStopTimes(start, departureTime);

    if (startOptions.empty())
        return Path();

    // populacja
    std::vector<std::vector<const StopTime*>> population;

    for (int i = 0; i < 30; i++)
    {
        std::vector<const StopTime*> individual;
        individual.push_back(startOptions[randomInt(0, startOptions.size() - 1)]);
		auto currentStopTime = individual.back();

        while (individual.back()->getStop() != end)
        {
			//weź następny przystanek z tej samej trasy
			auto nextStopTime = individual.back()->getTrip()->getStopTimes().at(individual.back()->getIndexInRoute() + 1);
			//wez opcje kontynuacji podrozy z tego przystanku
            auto options = network.getStopTimes(nextStopTime->getStop(), 
                                                nextStopTime->getTime());
			//jesli brak opcji, to przerwij
            if (options.empty())
                break;
			//50% szans, że wybierzemy kontynuacje tej samej trasy
            auto it = std::find_if(options.begin(), options.end(),
                [&](const auto& elem)
                {
                    return elem->getTrip() == currentStopTime->getTrip();
                });

            if (it != options.end() && randomInt(0, 100) < 50)
                individual.push_back(*it);
            else
			    individual.push_back(options[randomInt(0, options.size() - 1)]);
        }
        
        population.push_back(individual);
    }

    for (int gen = 0; gen < 100; gen++)
    {
        std::sort(population.begin(), population.end(),
            [&](auto& a, auto& b)
            {
                return fitness(a, end) < fitness(b, end);
            });

        // najlepszy zostaje
        std::vector<std::vector<const StopTime*>> newPop;
        newPop.push_back(population.front());

        while (newPop.size() < population.size())
        {
            auto& p1 = population[randomInt(0, 10)];
            auto& p2 = population[randomInt(0, 10)];

            auto child = crossover(p1, p2);

            if (randomInt(0, 100) < 20)
                mutate(child, startOptions);

            newPop.push_back(child);
        }

        population = std::move(newPop);
    }

    auto best = std::min_element(population.begin(), population.end(),
        [&](auto& a, auto& b)
        {
            return fitness(a, end) < fitness(b, end);
        });
	Path path(*best);

    return path;
}
