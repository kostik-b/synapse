#ifndef CLICK_TRADE_PROCESSOR_HH
#define CLICK_TRADE_PROCESSOR_HH

#include <click/element.hh>
#include <click/string.hh>

#include <click/timer.hh>
#include <click/global_sizes.hh>
#include <click/hashmap.hh>
#include <click/array_wrapper.hh>

CLICK_DECLS

struct TimeStats
{
  TimeStats ()
  {
    _first_time_updated = false;
    _size               = 0;
    _high = _low = _open = _close = 0;
  }

  fixedpt  _high;
  fixedpt  _low;
  fixedpt  _open;
  fixedpt  _close;
  int64_t _size;

  bool    _first_time_updated;

  void rollover ()
  {
    _open               = _close;
    _size               = 0;
    // high and low remain the same

    _first_time_updated = false;
  }
}; // struct TimeStats

class TradeProcessor : public Element
{
  public:

    typedef Synapse::ArrayWrapper<char, Synapse::ORDER_SYMBOL_LEN>  SymbolArrayWrapper;
    typedef HashMap<SymbolArrayWrapper, TimeStats>                  TimeStatsMap;
    // symbol to port
    typedef HashMap<SymbolArrayWrapper, int>                        SubscriptionsMap;


    TradeProcessor();
    ~TradeProcessor();

    const char *class_name() const		{ return "TradeProcessor"; }
    const char *port_count() const		{ return "1/-"; }

    int configure(Vector<String> &, ErrorHandler *);
    int initialize(ErrorHandler *);

    bool can_live_reconfigure() const		{ return false; }
    void add_handlers();

    void        push             (int                       port,
                                  Packet*                   p);

    void        run_timer        (Timer*                    timer);

  private:
    void        send_update_msg  (const TimeStats&          stats,
                                  const uint64_t            timestamp,
                                  int                       out_port,
                                  Packet&                   packet,
                                  const bool                is_init);

    void        send_add_msg     (const TimeStats&          stats,
                                  const int                 out_port);
  private:

    Timer             _timer;
    uint32_t          _aggregation_interval_sec;

    TimeStatsMap      _time_stats_map;
    SubscriptionsMap  _subscriptions_map;

    bool              _debug;
    bool              _active;

    uint64_t          _total_msgs;

    WritablePacket*   _w_packet;

#ifdef CLICK_LINUXMODULE
    bool              _cpu : 1;
#endif

}; // class TradeProcessor

CLICK_ENDDECLS
#endif
