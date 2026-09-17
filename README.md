# BusPathFinder

Research project developed as part of a master's thesis on **heuristic algorithms for multi-objective bus connection planning**.

The project investigates and compares three population-based optimization algorithms:

* **Genetic Algorithm (GA)**
* **Ant Colony Optimization (ACO)**
* **Particle Swarm Optimization (PSO)**

The algorithms search for bus connections between an origin and destination while optimizing multiple objectives simultaneously, such as:

* arrival time
* total travel time
* waiting time
* number of transfers

Solutions are evaluated using **Pareto dominance** and the **Hypervolume (HV)** indicator.

## Experiments

The project includes an experimental framework for:

* running repeated experiments with different algorithms and parameters
* processing and aggregating results
* calculating Hypervolume and success rate
* measuring execution time
* performing statistical analysis
* generating plots for result comparison

Experiments are performed on real public transport data in **GTFS** format.

## Technologies

* **C++**
* **nlohmann/json**
* **Python**
* **pandas**
* **NumPy**
* **SciPy**
* **pymoo**

## Purpose

The main goal is to experimentally compare the behavior, solution quality and computational performance of GA, ACO and PSO when solving a multi-objective bus connection planning problem.

The project is part of a master's thesis:

> **Comparative analysis of heuristic algorithms for bus connection planning**
