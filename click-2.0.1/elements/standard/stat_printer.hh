#ifndef CLICK_STAT_PRINTER_HH
#define CLICK_STAT_PRINTER_HH
#include <click/element.hh>
#include <click/string.hh>

#include <click/hashmap.hh>
#include <click/array_wrapper.hh>
#include <click/global_sizes.hh>
#include <click/cycles_counter.hh>
#include <click/running_stat.hh>

CLICK_DECLS

struct StatPair
{
  uint64_t  _start_tstamp;
  uint64_t  _end_tstamp;
  FixedPt   _value;
};

class StatPrinter : public Element
{
  public:
    StatPrinter();
    ~StatPrinter();

    const char *class_name() const		{ return "StatPrinter"; }
    const char *port_count() const		{ return "1-/-"; }

    int configure(Vector<String> &, ErrorHandler *);
    bool can_live_reconfigure() const		{ return false; }
    void add_handlers();

    Packet* simple_action (Packet* p);

  private:
    void update_stats             (Packet& p);
    void update_running_stat      (Packet& p);
    void update_running_stat_new  (Packet& p);
    void report_running_stat  ();

  private:
    Synapse::CyclesCounter  _cycles_counter;
    Synapse::RunningStat    _running_stat;
    int                     _reporting_interval;
    unsigned                _all_values_counter;

    bool                    _active;
    bool                    _combined_mode;
    bool                    _longest_indic;

  // MT
    unsigned                _mt_arr_idx;

#ifdef CLICK_LINUXMODULE
    bool                    _cpu : 1;
#endif

}; // class StatPrinter

CLICK_ENDDECLS
#endif
