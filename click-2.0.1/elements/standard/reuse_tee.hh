#ifndef CLICK_REUSE_TEE_HH
#define CLICK_REUSE_TEE_HH
#include <click/element.hh>
CLICK_DECLS

/*
 * =c
 * ReuseTee([N])
 *
 * PullReuseTee([N])
 * =s basictransfer
 * duplicates packets
 * =d
 * ReuseTee sends a copy of each incoming packet out each output.
 *
 * PullReuseTee's input and its first output are pull; its other outputs are push.
 * Each time the pull output pulls a packet, it
 * sends a copy out the push outputs.
 *
 * ReuseTee and PullReuseTee have however many outputs are used in the configuration,
 * but you can say how many outputs you expect with the optional argument
 * N.
 */

static const size_t COPY_MSG_MAX_LEN = 256;

class ReuseTee : public Element {

 public:

  ReuseTee();
  ~ReuseTee();

  const char *class_name() const		{ return "ReuseTee"; }
  const char *port_count() const		{ return "1/1-"; }
  const char *processing() const		{ return PUSH; }

  int configure(Vector<String> &, ErrorHandler *);

  void push(int, Packet *);

 private:
  char _copy_buffer[COPY_MSG_MAX_LEN];

};

CLICK_ENDDECLS
#endif
