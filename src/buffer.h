#ifndef BUFFER_H
#define BUFFER_H
#include <assert.h>
#include <memory>
#include <queue>
#include <ramulator_wrapper.h>
#include <string>
#include <types.h>

class Buffer_base {
public:
  std::tuple<unsigned, unsigned, unsigned, unsigned> get_current_location() {
    return {x, y, z, l};
  }
  std::tuple<unsigned, unsigned, unsigned, unsigned> get_next_location() {
    return {nx, ny, nz, nl};
  }
  void
  set_current_location(std::tuple<unsigned, unsigned, unsigned, unsigned> t) {
    auto [_x, _y, _z, _l] = t;
    x = _x;
    y = _y;
    z = _z;
    l = _l;
  }
  void set_next_location(std::tuple<unsigned, unsigned, unsigned, unsigned> t) {
    auto [_x, _y, _z, _l] = t;
    nx = _x;
    ny = _y;
    nz = _z;
    nl = _l;
  }
  Buffer_base(const std::string name) : name(name) {}

private:
  unsigned x, y, z, l;
  unsigned nx, ny, nz, nl;
  std::string name;
};
class Aggregator_buffer : Buffer_base {
public:
  Aggregator_buffer();
  Aggregator_buffer(const std::string name) : Buffer_base(name) {}
  // this is the special buffer for the aggregator and comb,
  // the aggregator put the result in
  // the comb comsume the buffer.
  void set_next(std::shared_ptr<Req> req);

  // the next is ready to move
  void complete_next();
  void complete_current();
  void move();

  bool is_current_ready();
  bool is_current_empty();
  bool is_next_empty();
  bool is_next_ready();

private:
  std::shared_ptr<Req> current_task;
  std::shared_ptr<Req> next_task;
  //
  // which means data is finished process, ready to erase or move
  bool current_ready, next_ready;
  bool current_empty, next_emtpy;
  //
};

class WriteBuffer : Buffer_base {
public:
  WriteBuffer(const std::string name) : Buffer_base(name) {}
  // start to write to the
  void start_write(std::shared_ptr<Req> req) {
    assert(!next_req);
    assert(next_buffer_empty);
    assert(!next_buffer_finished);
    next_req = req;
    next_buffer_empty = false;
    next_buffer_finished = false;
  }
  void end_write() {
    assert(next_req);
    assert(!next_buffer_empty);
    assert(!next_buffer_finished);
    next_buffer_finished = true;
  }
  void move() {
    assert(!next_buffer_empty and next_buffer_finished);
    assert(!current_buffer_ready);
    current_buffer_ready = true;
    current_buffer_start_send = false;
    current_buffer_finished = false;
    current_req = next_req;
    next_req = nullptr;
  }
  std::shared_ptr<Req> pop_next() {
    assert(current_buffer_ready and current_buffer_start_send);
    assert(!current_buffer_finished);
    current_buffer_finished = true;
    current_buffer_ready = false;
    current_buffer_start_send = false;

    return current_req;
  }
  bool is_current_send_ready() {
    return current_buffer_ready and current_buffer_start_send and
           !current_buffer_finished;
  }

  void cycle() {
    if (current_buffer_ready and !current_buffer_start_send) {
      current_buffer_start_send = true;
      assert(current_buffer_finished == false);
    }
  }
  bool is_next_empty() { return next_buffer_empty; }

private:
  std::shared_ptr<Req> current_req;
  std::shared_ptr<Req> next_req;
  bool current_buffer_ready, current_buffer_start_send, current_buffer_finished;
  bool next_buffer_empty, next_buffer_finished;
};

// INPUT:
// send the req into the input queue,
// and call buffer.cycle
// when the data is ready the current_data_ready will be true,
// remeber to send the mem request out and
class Buffer : Buffer_base {
private:
  unsigned long long current_addr = 0;
  unsigned current_lenghth = 0;
  bool current_data_ready = false;
  bool current_task_ready = false;
  bool current_task_sent = false;

  unsigned long long next_addr = 0;
  unsigned next_lenghth = 0;
  bool next_data_ready = false;
  bool next_task_ready = false;
  bool next_task_sent = false;

  std::queue<std::shared_ptr<Req>> in_task_queu;
  std::shared_ptr<Req> current_buffer_task;
  std::shared_ptr<Req> next_buffer_task;
  std::queue<std::shared_ptr<Req>> out_send_queue;
  std::queue<std::shared_ptr<Req>> ret_queue;
  std::shared_ptr<ramulator_wrapper> m_ramulator;

  // TODO write send to dram logic

public:
  void clear() {
    current_addr = 0;
    current_lenghth = 0;
    current_data_ready = false;
    current_task_ready = false;
    current_task_sent = false;

    next_addr = 0;
    next_lenghth = 0;
    next_data_ready = false;
    next_task_ready = false;
    next_task_sent = false;
  }
  void send(std::shared_ptr<Req> req) { in_task_queu.push(req); }
  void cycle();

  // when next buffer is ready, move it to current
  void add_task_and_move(unsigned long long addr, unsigned lenghth);
  // move the next buffer to current, and do not add new task.
  void just_move_the_buffer() {
    current_addr = next_addr;
    current_lenghth = next_lenghth;
    current_buffer_task = next_buffer_task;
    next_buffer_task = nullptr;

    current_data_ready = next_data_ready;
    current_task_ready = next_task_ready;
    current_task_sent = next_task_sent;

    next_task_ready = false;
    next_data_ready = false;
    next_task_sent = false;
  }
  bool is_out_send_q_ready() { return !out_send_queue.empty(); }
  std::shared_ptr<Req> get_out_send_req() { return out_send_queue.front(); }
  std::shared_ptr<Req> pop_out_send_req() {
    auto req = get_out_send_req();
    out_send_queue.pop();
    return req;
  }
  unsigned long long get_current_addr() { return current_addr; }
  unsigned get_current_lenghth() { return current_lenghth; }
  bool is_current_data_ready() { return current_data_ready; }
  bool is_current_task_ready() { return current_task_ready; }
  bool is_current_empty() { return !current_task_ready; }
  bool is_current_sent() { return current_task_sent; }

  unsigned long long get_next_addr() { return next_addr; }
  unsigned get_next_lenghth() { return next_lenghth; }
  bool is_next_data_ready() { return next_data_ready; }
  bool is_next_task_ready() { return next_task_ready; }
  bool is_next_empty() { return !next_task_ready; }
  bool is_next_sent() { return next_task_sent; }

  void set_current_data_ready() {
    assert(current_data_ready == false);
    current_data_ready = true;
  }
  void set_current_task_ready() {
    assert(current_task_ready == false);
    current_task_ready = true;
  }
  void set_next_data_ready() {
    assert(next_data_ready == false);
    next_data_ready = true;
  }
  void set_next_task_ready() {
    assert(next_task_ready == false);
    next_task_ready = true;
  }

  void insert_next(std::shared_ptr<Req> r) {
    next_task_ready = true;
    next_buffer_task = r;
    assert(next_task_sent = false);
    assert(next_data_ready = false);
  }

  Buffer(const std::string name) : Buffer_base(name) {}
};
#endif /* BUFFER_H */
