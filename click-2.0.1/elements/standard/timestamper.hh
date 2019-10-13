#ifndef CLICK_SYNAPSE_TIMESTAMPER_HH
#define CLICK_SYNAPSE_TIMESTAMPER_HH
#include <click/element.hh>
#include <click/string.hh>

#include <click/hashmap.hh>
#include <click/array_wrapper.hh>
#include <click/global_sizes.hh>
#include <click/cycles_counter.hh>
#include <click/running_stat.hh>

CLICK_DECLS


class Timestamper : public Element
{
  public:
    Timestamper();
    ~Timestamper();

    const char *class_name() const		{ return "Timestamper"; }
    const char *port_count() const		{ return "1/1"; }

    int configure(Vector<String> &, ErrorHandler *);
    bool can_live_reconfigure() const		{ return false; }
    void add_handlers();

    void    print_avg_latency (const uint64_t tstamp);
    Packet* simple_action     (Packet*        p);

  private:
    bool                    _active;

    bool                    _print_avg;
    bool                    _replace_tstamp;
    uint64_t                _total_cycles;
    uint64_t                _running_count;

#ifdef CLICK_LINUXMODULE
    bool                    _cpu : 1;
#endif

}; // class Timestamper

CLICK_ENDDECLS
#endif
