#ifndef CLICK_CHIX_TRADE_AGGREGATOR_HH
#define CLICK_CHIX_TRADE_AGGREGATOR_HH
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
    reset ();
  }

  double  _high;
  double  _low;
  double  _open;
  double  _close;
  int64_t _size;

  bool    _first_time_updated;

  void reset ()
  {
    _high               = 0.0f;
    _low                = 0.0f;
    _open               = 0.0f;
    _close              = 0.0f;
    _size               = 0;

    _first_time_updated = false;
  }
}; // struct TimeStats

class TradeAggregator : public Element
{
  public:

    typedef Synapse::ArrayWrapper<char, Synapse::ORDER_SYMBOL_LEN>  SymbolArrayWrapper;
    typedef HashMap<SymbolArrayWrapper, TimeStats>                  TimeStatsMap;


    TradeAggregator();
    ~TradeAggregator();

    const char *class_name() const		{ return "TradeAggregator"; }
    const char *port_count() const		{ return "1/1"; }

    int configure(Vector<String> &, ErrorHandler *);
    int initialize(ErrorHandler *);

    bool can_live_reconfigure() const		{ return false; }
    void add_handlers();

    void        push             (int                       port,
                                  Packet*                   p);

    void        run_timer        (Timer*                    timer);

  private:
    void        update_stats     (TimeStats&                time_stats,
                                  const Synapse::MsgTrade&  msg_trade);

    void        send_stats_msg   (const SymbolArrayWrapper& symbol,
                                  const TimeStats&          stats);

  private:

    Timer         _timer;
    uint32_t      _aggregation_interval_sec;

    TimeStatsMap  _time_stats_map;

    bool          _debug;
    bool          _active;
#ifdef CLICK_LINUXMODULE
    bool          _cpu : 1;
#endif

}; // class TradeAggregator

CLICK_ENDDECLS
#endif
