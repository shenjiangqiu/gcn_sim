#ifndef BUFFER_H
#define BUFFER_H
#include <string>

class buffer {
private:
  std::string name;

  unsigned long long current_addr = 0;
  unsigned current_lenghth = 0;
  bool current_ready = false;
  bool current_task_ready = false;

  unsigned long long next_addr = 0;
  unsigned next_lenghth = 0;
  bool next_ready = false;
  bool next_task_ready = false;

public:
  buffer(std::string name) : name(name) {}
  void add_task_and_move(unsigned long long addr, unsigned lenghth);

  void get_current_addr();
  void get_current_lenghth();
  bool is_current_ready();
  bool is_current_task_ready();

  void get_next_addr();
  void get_next_lenghth();
  bool is_next_ready();
  bool is_next_task_ready();

  void current_data_ready();
  void next_data_ready();
  

};
#endif /* BUFFER_H */
