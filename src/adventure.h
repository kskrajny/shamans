#ifndef SRC_ADVENTURE_H_
#define SRC_ADVENTURE_H_

#include <algorithm>
#include <vector>

#include "../third_party/threadpool/threadpool.h"

#include "./types.h"
#include "./utils.h"


class Adventure {
 public:
  virtual ~Adventure() = default;

  virtual uint64_t packEggs(std::vector<Egg>& eggs, BottomlessBag& bag) = 0;

  virtual void arrangeSand(std::vector<GrainOfSand>& grains) = 0;

  virtual Crystal selectBestCrystal(std::vector<Crystal>& crystals) = 0;
};

class LonesomeAdventure : public Adventure {
 public:
  LonesomeAdventure() {}

  virtual uint64_t packEggs(std::vector<Egg>& eggs, BottomlessBag& bag) {
      uint64_t S = bag.getCapacity()+1;
      uint64_t n = eggs.size()+1;
      // B[i][j] - true, jesli jajo i-1 powinno byc wstawione do plecaka o pojemnosci j
      std::vector<std::vector<bool>> B(n, std::vector<bool>(S, false));
      // A[i][j] - maks waga zebranych jaj ze zbioru jaj o indeksach {0,1,...,i-1} do plecaka o pojemnosci j
      std::vector<std::vector<uint64_t>> A(n, std::vector<uint64_t>(S, 0));
      for (uint64_t j=0;j<S;j++) {
          for(uint64_t i=1;i<n;i++) {
              uint64_t size = eggs[i-1].getSize();
              if (size > j) {
                  A[i][j] = A[i-1][j];
              } else {
                  A[i][j] = std::max(A[i-1][j], A[i-1][j-size] + eggs[i-1].getWeight());
                  if(A[i][j] > A[i-1][j]) B[i][j] = true;
              }
          }
      }
      uint64_t s = S-1;
      uint64_t i = n-1;
      // odtwarzanie zebranych jaj i wrzucenie ich do plecaka
      while(i != 0) {
          if(B[i][s]) {
              bag.addEgg(eggs[i-1]);
              s -= eggs[i-1].getSize();
          }
          i--;
      }
      return A[n-1][S-1];
  }

  virtual void arrangeSand(std::vector<GrainOfSand>& grains) {
    std::sort(grains.begin(), grains.end());
  }

  virtual Crystal selectBestCrystal(std::vector<Crystal>& crystals) {
      if(crystals.size() == 0) throw std::exception();
      Crystal best = crystals[0];
      for(size_t i=1;i<crystals.size();i++) {
          best = best < crystals[i] ? crystals[i] : best;
      }
      return best;
  }
};

class TeamAdventure : public Adventure {
private:

    uint64_t numberOfShamans;
    ThreadPool councilOfShamans;

public:
  explicit TeamAdventure(uint64_t numberOfShamansArg)
      : numberOfShamans(numberOfShamansArg),
        councilOfShamans(numberOfShamansArg)
        {};

  typedef std::vector<uint64_t> col;
  typedef std::vector<col> matrix;
  typedef std::shared_ptr<matrix> shr;

  typedef std::vector<bool> colB;
  typedef std::vector<colB> matrixB;
  typedef std::shared_ptr<matrixB> shrB;

  static void findEgg(const uint64_t& len, const uint64_t& e, size_t f, size_t r,
          shr A, shrB B, std::vector<Egg>& eggs, TeamAdventure* team, uint64_t sha) {
      if(r-f <= len) {
          for(uint64_t i=f;i<=r;i++) {
              uint64_t size = eggs[e-1].getSize();
              if (size > i) {
                  A->at(e)[i] = A->at(e-1)[i];
              } else {
                  A->at(e)[i] = std::max(A->at(e-1)[i], A->at(e-1)[i-size] + eggs[e-1].getWeight());
                  if(A->at(e)[i] > A->at(e-1)[i]) B->at(e)[i] = true;
              }
          }
      } else {
          uint64_t floor = sha/2;
          uint64_t m = (r-f)*floor/sha + f;
          uint64_t help = sha;
          sha /= 2;
          auto x = team->councilOfShamans.enqueue(findEgg, len, e, f, m, A, B, eggs, team, sha);
          findEgg(len, e, m+1, r, A, B, eggs, team, help-sha);
          x.wait();
      }
  }

  uint64_t packEggs(std::vector<Egg>& eggs, BottomlessBag& bag) {
      uint64_t S = bag.getCapacity()+1;
      uint64_t n = eggs.size()+1;
      const uint64_t len = S/numberOfShamans+1;
      auto A = std::make_shared<matrix>(n, col(S, 0));
      auto B = std::make_shared<matrixB>(n, colB(S, false));
      for (uint64_t i=1;i<n;i++) {
          this->councilOfShamans.enqueue(findEgg, len, i, 0, S-1, A, B, eggs,
                  this, numberOfShamans).wait();
      }
      uint64_t s = S-1;
      uint64_t i = n-1;
      while(i != 0) {
          if(B->at(i)[s]) {
              bag.addEgg(eggs[i-1]);
              s -= eggs[i-1].getSize();
          }
          i--;
      }
      return A->at(n-1)[S-1];
  }

  static void sortGrains(const uint64_t& len, size_t f, size_t r, std::vector<GrainOfSand>* grains,
          TeamAdventure* team, uint64_t sha) {
      if(r-f <= len) {
          std::sort(grains->begin()+f,grains->begin()+r+1);
      } else {
          uint64_t floor = sha/2;
          uint64_t m = (r-f)*floor/sha + f;
          uint64_t help = sha;
          sha /= 2;
          auto x = team->councilOfShamans.enqueue(sortGrains, len, f, m, grains, team, sha);
          sortGrains(len, m+1, r, grains, team, help-sha);
          x.wait();
          std::vector<GrainOfSand> full;
          std::inplace_merge(grains->begin()+f, grains->begin()+m+1, grains->begin()+r+1,
                  [](const GrainOfSand& a, const GrainOfSand& b) {
              return (a < b);
          });
      }
  }

  virtual void arrangeSand(std::vector<GrainOfSand>& grains) {
      const uint64_t len = grains.size()/numberOfShamans+1;
      sortGrains(len, 0, grains.size()-1, &grains, this, numberOfShamans);
  }

    static Crystal findCrystal(const uint64_t& len, size_t f, size_t r, std::vector<Crystal>& crystals,
            TeamAdventure *team, uint64_t sha) {
        if(r-f <= len) {
            auto best = crystals[f];
            auto s = crystals.size();
            for(uint64_t i=f+1;i<r+1 && i<s;i++) {
                best = best < crystals[i] ? crystals[i] : best;
            }
            return best;
        } else {
            uint64_t floor = sha/2;
            uint64_t m = (r-f)*floor/sha + f;
            uint64_t help = sha;
            sha /= 2;
            auto x = team->councilOfShamans.enqueue(findCrystal, len, f, m, crystals, team, sha);
            auto y = findCrystal(len, m+1, r, crystals, team, help-sha);
            auto z = x.get();
            return z < y ? y : z;
        }
    }

  virtual Crystal selectBestCrystal(std::vector<Crystal>& crystals) {
      const uint64_t len = crystals.size()/numberOfShamans+1;
      return this->councilOfShamans.enqueue(findCrystal, len, 0, crystals.size()-1, crystals,
              this, numberOfShamans).get();
  }
};


#endif  // SRC_ADVENTURE_H_
