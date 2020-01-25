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
      //about size
      uint64_t W = bag.getCapacity()+1;
      uint64_t n = eggs.size()+1;
      std::vector<std::vector<uint64_t >> A(n, std::vector<uint64_t>(W, 0));
      for(uint64_t i=1;i<n;i++) {
          for (uint64_t j=0;j<W;j++) {
              uint64_t size = eggs[i-1].getSize();
              if (size > j) {
                  A[i][j] = A[i-1][j];
              } else {
                  A[i][j] = std::max(A[i-1][j], A[i-1][j-size] + eggs[i-1].getWeight());
              }
          }
      }
      return A[n-1][W-1];
  }

  virtual void arrangeSand(std::vector<GrainOfSand>& grains) {
    // TODO Implement this method
    throw std::runtime_error("Not implemented");
  }

  virtual Crystal selectBestCrystal(std::vector<Crystal>& crystals) {
    // TODO Implement this method
    throw std::runtime_error("Not implemented");
  }
};

class TeamAdventure : public Adventure {
private:

    //uint64_t numberOfShamans;
    ThreadPool councilOfShamans;

    class EggsHunter {
    public:

        std::vector<Egg> eggs;
        std::vector<size_t> best_stack;
        uint64_t max_weight;
        std::mutex mut;
        ThreadPool *councilOfShamans;
        std::atomic<uint64_t > jobs;
        std::condition_variable cv;
        std::mutex cv_m;

        EggsHunter(std::vector<Egg>& e, std::vector<size_t>& b, ThreadPool* t) {
            eggs = e;
            best_stack = b;
            max_weight = 0;
            councilOfShamans = t;
            jobs = 1;
        }

        class drakeWrap {// holds some values for function eggCount
        public:
            uint64_t start;
            uint64_t stop;
            uint64_t cap;
            uint64_t weight;
            std::vector<size_t> stack;

            explicit drakeWrap(size_t s, uint64_t c) {
                start = 0;
                stop = s;
                cap = c;
                weight = 0;
            }

            drakeWrap operator=(drakeWrap const& other) {
                this->start = other.start;
                this->stop = other.stop;
                this->cap = other.cap;
                this->weight = other.weight;
                this->stack = other.stack;
                return *this;
            }
        };

        static void eggsCount(drakeWrap wrap, EggsHunter *hunter) {
            if(wrap.start == wrap.stop) { // if tasks are divided
                uint64_t size = hunter->eggs[wrap.start].getSize();
                if(wrap.cap >= size) { // if elem can be added
                    //update drakeWrap
                    wrap.weight += hunter->eggs[wrap.start].getWeight();
                    wrap.cap -= size;
                    wrap.stack.push_back(wrap.start);
                    wrap.start++;
                    wrap.stop = hunter->eggs.size()-1;
                    if(wrap.start <= wrap.stop) {
                        hunter->jobs++;
                        hunter->councilOfShamans->enqueue(EggsHunter::eggsCount, wrap, hunter);
                    } else {
                        if(hunter->max_weight < wrap.weight) {
                            hunter->mut.lock();
                            hunter->max_weight = wrap.weight;
                            hunter->best_stack = wrap.stack;
                            hunter->mut.unlock();
                        }
                    }
                } else { // no more eggs
                    if(hunter->max_weight < wrap.weight) {
                        hunter->mut.lock();
                        hunter->max_weight = wrap.weight;
                        hunter->best_stack = wrap.stack;
                        hunter->mut.unlock();
                    }
                }
            } else {
                uint64_t mid = (wrap.start + wrap.stop)/2;
                drakeWrap w1 = wrap;
                drakeWrap w2 = wrap;
                w1.stop = mid;
                w2.start = mid+1;
                hunter->jobs += 2;
                hunter->councilOfShamans->enqueue(EggsHunter::eggsCount, w1, hunter);
                hunter->councilOfShamans->enqueue(EggsHunter::eggsCount, w2, hunter);
            }
            hunter->jobs--;
            if(hunter->jobs == 0) hunter->cv.notify_all();
        }
    };

public:
  explicit TeamAdventure(uint64_t numberOfShamansArg)
      : //numberOfShamans(numberOfShamansArg),
        councilOfShamans(numberOfShamansArg)
        {}

  uint64_t packEggs(std::vector<Egg>& eggs, BottomlessBag& bag) {
      std::vector<size_t> stack;
      EggsHunter hunter(eggs, stack, &councilOfShamans);
      EggsHunter::drakeWrap wrap(eggs.size()-1, bag.getCapacity());
      EggsHunter::eggsCount(wrap, &hunter);
      std::unique_lock<std::mutex> lk(hunter.cv_m);
      hunter.cv.wait(lk);
      for(auto &x : hunter.best_stack) {
          bag.addEgg(eggs[x]);
      }
      return hunter.max_weight;
  }

  virtual void arrangeSand(std::vector<GrainOfSand>& grains) {
    // TODO Implement this method
    throw std::runtime_error("Not implemented");
  }

  virtual Crystal selectBestCrystal(std::vector<Crystal>& crystals) {
    // TODO Implement this method
    throw std::runtime_error("Not implemented");
  }

};

#endif  // SRC_ADVENTURE_H_
