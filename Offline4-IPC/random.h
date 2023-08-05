#include <iostream>
#include <random>

int get_random_number() {
  std::random_device rd;
  std::mt19937 generator(rd());

  double lambda = 10000.234;

  std::poisson_distribution<int> poissonDist(lambda);

  return poissonDist(generator);
}