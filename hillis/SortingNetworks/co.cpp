#include "Random.h"
#include "Coevolution.h"
#include <vector>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <future>

using namespace std;

// const int popsize = 65536; // must be rootable
const int popsize = 900; // must be rootable
const int crossover = popsize / 2;
const int mutation = 1000;
const int inputsize = 8;
const int comparesize = 19;
const int testsize = 10;
const int max_generation = 10000;

int run() {
  Coevolution co(popsize, crossover, mutation, inputsize, comparesize, testsize);
  co.Evaluate();
  SortingNetwork sn;
  for (int gen = 1; gen <= max_generation; gen++) {
    if (gen % 100 == 0) {
      cout << "gen: " << gen << '\n';
      cout << "host: " << co.AverageHostFitness() << '\n';
      cout << "para: " << co.AverageParasiteFitness() << '\n';
      sn = co.AllTimeBest();
      // cout << sn.Test() << "%\n";
      // cout << "Size: " << sn.Size() << '\n';
      if (sn.Size() == comparesize) break; 
      cout << '\n';
    }
    co.Selection();
    co.Evaluate();
  }
  sn.Print(); cout << '\n';
  cout << "Size: " << sn.Size() << '\n';
  cout << '\n';
  return sn.Size();
}

signed main() {
  auto start = chrono::high_resolution_clock::now();

  int runs = 0;
  int size = 0;
  while (size != comparesize) {
    runs++;
    size = run();
  }

  cout << "runs: " << runs << '\n';

  auto stop = chrono::high_resolution_clock::now();
  auto duration = chrono::duration_cast<chrono::milliseconds>(stop-start);
  cout << "Time: " << duration.count() << "ms\n";
}
