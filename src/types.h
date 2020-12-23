#ifndef TYPES_H
#define TYPES_H

enum class device_types { input_buffer, aggregator, edge_buffer };
enum class mem_request { read, write };
struct Req {
  Req() {
    static unsigned global_id = 0;
    id = global_id;
    global_id++;
  }
  unsigned id;
  unsigned long long addr;
  unsigned long len;
  device_types t;
  mem_request req_type;
};

#endif /* TYPES_H */
