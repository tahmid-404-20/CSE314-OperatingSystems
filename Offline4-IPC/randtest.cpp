#include "random.h"
#include <iostream>

using namespace std;

int main() {
  for (int i = 0; i < 10; i++) {
    cout << get_random_number() % 15 << endl;
  }

  return 0;
}
