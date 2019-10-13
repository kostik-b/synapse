// Copyright Queen's University Belfast 2012

#ifndef CLICK_UNIQUE_TEE_HH
#define CLICK_UNIQUE_TEE_HH

#include <click/element.hh>
CLICK_DECLS

// not only it clones each packet, but also uniqueifies each one
// of them

class UniqueTee : public Element {

 public:

  UniqueTee();
  ~UniqueTee();

  const char *class_name() const		{ return "UniqueTee"; }
  const char *port_count() const		{ return "1/1-"; }
  const char *processing() const		{ return PUSH; }

  int configure(Vector<String> &, ErrorHandler *);

  void push(int, Packet *);

};

CLICK_ENDDECLS
#endif
