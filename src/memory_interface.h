#ifndef MEMORY_INTERFACE_H
#define MEMORY_INTERFACE_H
#include <assert.h>
#include <map>
#include <memory>
#include <queue>
#include <ramulator_wrapper.h>
#include <types.h>
class memory_interface {
private:
  unsigned waiting_size;
  std::queue<std::shared_ptr<Req>> req_queue;
  std::queue<std::pair<unsigned long long, bool>> out_send_queue;
  std::queue<unsigned long long> response_queue;
  std::queue<std::shared_ptr<Req>> task_return_queue;

  std::map<unsigned, unsigned> id_to_numreqs_map;
  std::map<unsigned long long, std::shared_ptr<Req>> addr_to_req_map;

public:
  void cycle();
  bool avaliable() { return req_queue.size() < waiting_size; }
  void send(std::shared_ptr<Req> req) {
    assert(req_queue.size() < waiting_size);
    req_queue.push(req);
    auto num_reqs = (req->len + 63) / 64;
    id_to_numreqs_map.insert({req->id, num_reqs});
  }
  std::pair<unsigned long long, bool> get_next() {
    return out_send_queue.front();
  }
  bool ret_avaliable() { return task_return_queue.size() > 0; }
  std::shared_ptr<Req> get_req() {
    auto ret = task_return_queue.front();
    task_return_queue.pop();
    return ret;
  }

  memory_interface();

  void cycle() {
    // send policy changed here
    if (!req_queue.empty()) {
      auto next_req = req_queue.front();
      if (next_req->len > 0) {
        addr_to_req_map.insert({next_req->addr, next_req});
        out_send_queue.push(
            {next_req->addr, next_req->req_type == mem_request::write});
        if (next_req->len - 64 <= 0) {
          req_queue.pop();
        } else {
          next_req->addr += 64;
          next_req->len -= 64;
        }
      }
    }
    if (!response_queue.empty()) {
      auto &resp = response_queue.front();
      auto req = addr_to_req_map.at(resp);
      addr_to_req_map.erase(resp);
      if (--id_to_numreqs_map.at(req->id) == 0) {
        id_to_numreqs_map.erase(req->id);
        task_return_queue.push(req);
      }
    }
  }
};
#endif /* MEMORY_INTERFACE_H */
