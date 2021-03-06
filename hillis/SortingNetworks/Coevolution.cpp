#include "Coevolution.h"
#include "Random.h"
#include <map>
#include <algorithm>
#include <math.h>
#include <thread>

#include <iostream>

using namespace std;

Coevolution::Coevolution(int popsize, int crossover, int mutation, int inputsize, int comparesize, int testsize) {
  population_size = popsize;
  height = width = sqrt(popsize);
  crossover_rate = crossover;
  mutation_rate = mutation;
  input_size = inputsize;
  compare_size = comparesize;
  test_size = testsize;

  // init host (sortingnetworks)
  host.resize(height);
  for (int i = 0; i < height; i++)
    for (int j = 0; j < width; j++)
      host[i].push_back(SortingNetwork(comparesize, inputsize, mutation));

  // making testcases
  vector<vector<int>> tests(1 << input_size);
  for (int bit = 0; bit < (1 << input_size); bit++) {
    vector<int> s(input_size);
    for (int i = 0; i < input_size; i++)
      s[i] = bit & 1<<i ? 1 : 0;
    tests[bit] = s;
  }
  // init parasite
  parasite.resize(height);
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      // TestCases t(testsize, inputsize, mutation_rate, tests);
      TestCases t(testsize, inputsize, mutation_rate);
      parasite[i].push_back(t);
    }
  }
}

void Coevolution::CalculateFitness(SortingNetwork &sn, TestCases &tc) {
  float fitness = 0;
  sn.Merge();
  tc.Merge();
  for (auto t : tc.testcases) {
    auto a = t;
    sort(a.begin(), a.end());
    sn.Sort(t);
    if (t == a) fitness++;
  }
  fitness /= tc.Size();
  // if (fitness == 1) fitness += 0.01 * (compare_size*2 - sn.Size());
  sn.SetFitness(fitness);
  tc.SetFitness(1-fitness);
}

void Coevolution::Evaluate() {
  for (int i = 0; i < height; i++) {
    vector<thread> threads;
    for (int j = 0; j < width; j++)
      Coevolution::CalculateFitness(host[i][j], parasite[i][j]);
      // threads.emplace_back(&Coevolution::CalculateFitness, this, ref(host[i][j]), ref(parasite[i][j]));
    // for (auto &t : threads) t.join();
  }
}

const int di[] = {1, 0, -1, 0};
const int dj[] = {0, 1, 0, -1};
void Coevolution::Selection() {
  // create gametes
  for (int i = 0; i < height; i++) for (int j = 0; j < width; j++) {
    host[i][j].CreateGametes();
    parasite[i][j].CreateGametes();
  }

  // vector to sort population by fitness and size
  vector<pair<pair<float,int>,pair<int,int>>> v(population_size);
  // host
  // sort host population
  for (int i = 0; i < height; i++) for (int j = 0; j < width; j++)
    v[i*height+j] = {{host[i][j].Fitness(), -host[i][j].Size()}, {i, j}};
  sort(v.begin(), v.end());
  // cull bottom half
  for (int h = 0; h < population_size/2; h++) {
    auto [i, j] = v[h].second;
    for (int d = 0; d < 4; d++) {
      int ni = (i+di[d]+height) % height;
      int nj = (j+dj[d]+width) % width;
      if (host[i][j].Fitness() < host[ni][nj].Fitness())
        host[i][j] = host[ni][nj];
    }
  }
  // walk around grid to find mate
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {

      // save top 10
      // bool skip = false;
      // for (int k = 0; k < 5; k++)
        // if (i == v[population_size-k-1].second.first and j == v[population_size-k-1].second.second)
          // skip = true;
      // if (skip) continue;

      int ni = i, nj = j;
      while (fastrng() % 2) {
        int r = fastrng() % 4;
        ni = (i+di[r]+height) % height;
        nj = (j+dj[r]+width) % width;
      }
      // crossover
      host[i][j].Crossover(host[ni][nj]);
    }
  }

  // adding best network
  for (int k = 0; k < 1; k++) {
    auto [i, j] = v[population_size-k-1].second;
    best.push_back(host[i][j]);
  }

  // parasite
  //sort parasites
  for (int i = 0; i < height; i++) for (int j = 0; j < width; j++)
    v[i*height+j] = {{parasite[i][j].Fitness(), -parasite[i][j].Size()}, {i, j}};
  sort(v.begin(), v.end());
  // cull bottom half
  for (int h = 0; h < population_size/2; h++) {
    auto [i, j] = v[h].second;
    for (int d = 0; d < 4; d++) {
      int ni = (i+di[d]+height) % height;
      int nj = (j+dj[d]+width) % width;
      if (parasite[i][j].Fitness() < parasite[ni][nj].Fitness())
        parasite[i][j] = parasite[ni][nj];
    }
  }
  // walk around grid
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      // if (i == v.back().second.first and j == v.back().second.second) continue;
      int ni = i, nj = j;
      while (fastrng() % 2) {
        int r = fastrng() % 4;
        ni = (i+di[r]+height) % height;
        nj = (j+dj[r]+width) % width;
      }
      // crossover
      parasite[i][j].Crossover(parasite[ni][nj]);
    }
  }
}

float Coevolution::AverageHostFitness() {
  float sum = 0;
  for (int i = 0; i < height; i++)
    for (int j = 0; j < width; j++)
      sum += host[i][j].Fitness();
  return sum / population_size;
}

float Coevolution::AverageParasiteFitness() {
  float sum = 0;
  for (int i = 0; i < height; i++)
    for (int j = 0; j < width; j++)
      sum += parasite[i][j].Fitness();
  return sum / population_size;
}

SortingNetwork Coevolution::GetBestNetwork() {
  int best_i = 0, best_j = 0;
  for (int i = 0; i < height; i++) for (int j = 0; j < width; j++) {
    if (host[i][j].Fitness() > host[best_i][best_j].Fitness()) {
      best_i = i;
      best_j = j;
    }
    if (host[i][j].Fitness() == host[best_i][best_j].Fitness() and host[i][j].Size() < host[best_i][best_j].Size()) {
      best_i = i;
      best_j = j;
    }
  }
  return host[best_i][best_j];
}

SortingNetwork Coevolution::AllTimeBest() {
  vector<pair<pair<float,int>,int>> v(best.size());
  for (int i = 0; i < (int)best.size(); i++) {
    if (best[i].Accuracy() < 0) best[i].SetAccuracy(best[i].Test());
    v[i] = {{best[i].Accuracy(), -best[i].Size()}, i};
    // v[i] = {{best[i].Test(), -best[i].Size()}, i};
  }
  sort(v.rbegin(), v.rend());

  auto ret = best[v[0].second];

  vector<SortingNetwork> new_best;
  for (int i = 0; i < (int)best.size(); i++)
    if (v[i].first.first == 100)
      new_best.push_back(best[v[i].second]);
  swap(best, new_best);

  cout << best.size() << '\n';
  for (int i = 0; i < 5; i++)
    cout << v[i].first.first << ' ' << -v[i].first.second << '\n';

  return ret;
}

void Coevolution::PrintHost() {
  for (auto &p : host) {
    for (auto &x : p)
      cout << x.Fitness() << ' ';
    cout << '\n';
  }
}
void Coevolution::PrintParasite() {
  for (auto &p : parasite) {
    for (auto &x : p)
      cout << x.Fitness() << ' ';
    cout << '\n';
  }
}
