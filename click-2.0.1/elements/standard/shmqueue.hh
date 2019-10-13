#ifndef CLICK_SHM_ELEMENT_HH
#define CLICK_SHM_ELEMENT_HH
#include <click/element.hh>
#include <click/string.hh>

CLICK_DECLS


class ShmQueue : public Element
{
  public:
    ShmQueue();
    ~ShmQueue();

    const char *class_name() const		{ return "ShmQueue"; }
    const char *port_count() const		{ return "1/0"; }

    int configure(Vector<String> &, ErrorHandler *);
    bool can_live_reconfigure() const		{ return false; }
    void add_handlers();

    void push(int port, Packet *p);

  private:

    bool            _active;
    key_t           _shmq_key;
    int             _shmq_id;
#ifdef CLICK_LINUXMODULE
    bool          _cpu : 1;
#endif

}; // class ShmQueue

CLICK_ENDDECLS
#endif
