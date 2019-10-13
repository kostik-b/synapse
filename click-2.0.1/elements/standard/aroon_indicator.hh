#ifndef CLICK_AROON_INDICATOR_HH
#define CLICK_AROON_INDICATOR_HH
#include <click/element.hh>
#include <click/string.hh>

#include <boost/circular_buffer.hpp>
#include <click/global_sizes.hh>
#include <click/hashmap.hh>
#include <click/timer.hh>
#include <click/array_wrapper.hh>

CLICK_DECLS

class AroonIndicator
{
  public:
    typedef boost::circular_buffer<double>  CircularBuffer;

    AroonIndicator ()
      : _aroon_up_indicator   (0.0f)
      , _aroon_down_indicator (0.0f)
      , _aroon_time_span      (30.0)
      , _price_buffer        (_aroon_time_span)
    {}

    void update_indicator (const double price)
    {
      // just the first update
      _price_buffer.push_back (price);

      if (_price_buffer.size () < _aroon_time_span)
      {
        return;
      }

      CircularBuffer::iterator iter = _price_buffer.begin ();
      CircularBuffer::iterator end  = _price_buffer.end ();

      int     ticksSinceMinPrice  = 29;
      int     ticksSinceMaxPrice  = 29;
      double  minPrice            = *iter;
      double  maxPrice            = *iter;
      int     backwardCounter     = 29;

      while ((iter != end) && (backwardCounter > -1))
      {
        double currentPrice = *iter;

        if (currentPrice >= maxPrice)
        {
          maxPrice            = currentPrice;
          ticksSinceMaxPrice  = backwardCounter;
        }
        else if (currentPrice <= minPrice)
        {
          minPrice            = currentPrice;
          ticksSinceMinPrice  = backwardCounter;
        }

        ++iter;
        --backwardCounter;
      }

      // recalculate indicators
      _aroon_up_indicator     = 100.0 * (1.0 - (double(ticksSinceMaxPrice) / _aroon_time_span));
      _aroon_down_indicator   = 100.0 * (1.0 - (double(ticksSinceMinPrice) / _aroon_time_span));
    }

    double get_aroon_up_indicator ()
    {
      return _aroon_up_indicator;
    }

    double get_aroon_down_indicator ()
    {
      return _aroon_down_indicator;
    }

    double get_aroon_oscillator ()
    {
      return _aroon_up_indicator - _aroon_down_indicator;
    }

  private:
    double          _aroon_up_indicator;
    double          _aroon_down_indicator;

    double          _aroon_time_span;
    CircularBuffer  _price_buffer;
}; // class AroonIndicator

struct SymbolDataHolder
{
  SymbolDataHolder ()
  {
    _file_struct = NULL;
  }

  AroonIndicator  _aroon_indicator;
  FILE*           _file_struct;
}; // struct SymbolDataHolder


class AroonIndicatorElement : public Element
{
  public:
    typedef Synapse::ArrayWrapper<char, Synapse::ORDER_SYMBOL_LEN>  SymbolArrayWrapper;
    typedef HashMap<SymbolArrayWrapper, SymbolDataHolder>           AroonIndicatorMap;

    AroonIndicatorElement();
    ~AroonIndicatorElement();

    const char *class_name() const		{ return "AroonIndicatorElement"; }
    const char *port_count() const		{ return "1/1"; }

    int configure(Vector<String> &, ErrorHandler *);
    int initialize(ErrorHandler *);

    bool can_live_reconfigure() const		{ return false; }
    void add_handlers();

    Packet *simple_action(Packet *);

    void    run_timer    (Timer* timer);

  private:
    SymbolDataHolder* get_symbol_data (const char(& symbol)[Synapse::ORDER_SYMBOL_LEN]);

    void              write_indicators(const char(& symbol)[Synapse::ORDER_SYMBOL_LEN],
                                       const double price,
                                       const double aroon_up_indicator,
                                       const double aroon_down_indicator,
                                       FILE*&       file_struct);

  private:

    Timer             _timer;
    uint32_t          _file_flush_interval_sec;

    AroonIndicatorMap _indicator_map;
    bool              _active;
#ifdef CLICK_LINUXMODULE
    bool              _cpu : 1;
#endif

}; // class AroonIndicatorElement

CLICK_ENDDECLS
#endif
