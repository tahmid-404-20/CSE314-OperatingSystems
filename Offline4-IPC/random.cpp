#include <iostream>
#include <random>

int main() {
    // Set up a random number generator
    std::random_device rd;
    std::mt19937 generator(rd());

    // Set the mean (lambda) for the Poisson distribution
    double lambda = 1000.0;

    // Create a Poisson distribution with the specified lambda
    std::poisson_distribution<int> poissonDist(lambda);

    // Generate random numbers from the Poisson distribution and print them
    for (int i = 0; i < 10; ++i) {
        int randomNumber = poissonDist(generator);
        std::cout << "Random number " << i + 1 << ": " << randomNumber << std::endl;
    }

    return 0;
}