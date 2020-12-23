#ifndef RAMULATOR_WRAPPER_H
#define RAMULATOR_WRAPPER_H
#include "Cache.h"
#include "Memory.h"
#include "RamulatorConfig.h"
#include "Request.h"
#include "Statistics.h"
#include <ctype.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include <map>
#include <queue>
#include <set>
#include <tuple>
class ramulator_wrapper {
public:
  void send(uint64_t addr, bool is_read);
  void tick();
  void finish();
  ramulator_wrapper(const ramulator::Config configs, int cacheline);
  ~ramulator_wrapper();
  void call_back(ramulator::Request &req);
  bool empty() const;
  std::string get_internal_size() const;
  std::string get_line_trace() const;

  uint64_t get() const { return out_queue.front(); }
  uint64_t pop() {
    auto ret = out_queue.front();
    out_queue.pop();
    return ret;
  }
  bool return_avaliable() const { return !out_queue.empty(); }

protected:
  bool do_cycle();

private:
  double tCK;
  unsigned long long outgoing_reqs = 0;
  std::queue<std::pair<uint64_t, bool>> in_queue;
  std::queue<uint64_t> out_queue;
  ramulator::MemoryBase *mem;
};

#endif