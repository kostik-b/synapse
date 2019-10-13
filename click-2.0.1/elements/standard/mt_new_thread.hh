#ifndef CLICK_MT_TEE_HH
#define CLICK_MT_TEE_HH
#include <click/element.hh>
#include <click/task.hh>
#include <click/circ_array.hpp>
// #include <click/notifier.hh>
CLICK_DECLS


class MtNewThread : public Element { public:

    MtNewThread();
    ~MtNewThread();

    const char *class_name() const		{ return "MtNewThread"; }
    const char *port_count() const		{ return PORTS_1_1; }
    const char *processing() const		{ return AGNOSTIC; }

    int configure(Vector<String> &, ErrorHandler *);
    int initialize(ErrorHandler *);
//    void add_handlers();

    void push(int, Packet *);
    bool run_task(Task *);

  private:

    bool _active;
    bool _debug;
    Task _task;

    typedef CircArray<WritablePacket*> PacketsBuffer;
    PacketsBuffer _buffer;
    Spinlock      _spinlock;
//    NotifierSignal _signal;

};

CLICK_ENDDECLS
#endif
