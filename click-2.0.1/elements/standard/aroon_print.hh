#ifndef CLICK_AROON_PRINT_HH
#define CLICK_AROON_PRINT_HH
#include <click/element.hh>
#include <click/string.hh>
#include <click/hashmap.hh>

#include <boost/circular_buffer.hpp>
CLICK_DECLS

/*
=c

Print([LABEL, MAXLENGTH, I<keywords>])

=s debugging

prints packet contents

=d

Prints up to MAXLENGTH bytes of data from each packet, in hex, preceded by the
LABEL text. Default MAXLENGTH is 24.

Keyword arguments are:

=over 8

=item MAXLENGTH

Maximum number of content bytes to print. If negative, print entire
packet. Default is 24.

=item CONTENTS

Determines whether the packet data is printed.  May be `NONE' (do not print
packet data), `HEX' (print packet data in hexadecimal), or `ASCII' (print
packet data in plaintext).  Default is `HEX'.

=item TIMESTAMP

Boolean.  If true, prints each packet's timestamp in seconds since
1970.  Default is false.

=item PRINTANNO

Boolean.  If true, prints each packet's user annotation bytes.  Default is
false.

=item CPU

Boolean; available only in the Linux kernel module.  If true, prints the
current CPU ID for every packet.  Default is false.

=item HEADROOM

Boolean.  If true, prints each packet's headroom and tailroom, in the format
"(h[headroom] t[tailroom])".  Defaut is false.

=item ACTIVE

Boolean.  If false, don't print messages.  Default is true.

=back

=h active read/write

Returns or sets the ACTIVE parameter.

=a

IPPrint */

class AroonIndicator
{
  public:
    typedef boost::circular_buffer<double>  CircularBuffer;

    AroonIndicator ()
      : _aroonUpIndicator   (0.0f)
      , _aroonDownIndicator (0.0f)
      , _aroonTimeSpan      (30.0)
      , _priceBuffer        (_aroonTimeSpan)
    {}

    void updateIndicator (const double price)
    {
      // just the first update
      _priceBuffer.push_back (price);

      if (_priceBuffer.size () < _aroonTimeSpan)
      {
        return;
      }

      CircularBuffer::iterator iter = _priceBuffer.begin ();
      CircularBuffer::iterator end  = _priceBuffer.end ();

      int     ticksSinceMinPrice  = 29;
      int     ticksSinceMaxPrice  = 29;
      double  minPrice            = *iter;
      double  maxPrice            = *iter;
      int     backwardCounter     = 29;

      while ((iter != end) && (backwardCounter > -1))
      {
        double currentPrice = *iter;

        if (currentPrice > maxPrice)
        {
          maxPrice            = currentPrice;
          ticksSinceMaxPrice  = backwardCounter;
        }
        else if (currentPrice < minPrice)
        {
          minPrice            = currentPrice;
          ticksSinceMinPrice  = backwardCounter;
        }

        ++iter;
        --backwardCounter;
      }

      // recalculate indicators
      _aroonUpIndicator     = 100.0 * (1.0 - (double(ticksSinceMaxPrice) / _aroonTimeSpan));
      _aroonDownIndicator   = 100.0 * (1.0 - (double(ticksSinceMinPrice) / _aroonTimeSpan));
    }

    double getAroonUpIndicator ()
    {
      return _aroonUpIndicator;
    }

    double getAroonDownIndicator ()
    {
      return _aroonDownIndicator;
    }

    double getAroonOscillator ()
    {
      return _aroonUpIndicator - _aroonDownIndicator;
    }

  private:
    double          _aroonUpIndicator;
    double          _aroonDownIndicator;

    const double    _aroonTimeSpan;
    CircularBuffer  _priceBuffer;
}; // class AroonIndicator


struct OrderInfo
{
  OrderInfo ()
    : _price (0.0f)
    , _shares (0)
    , _counter(0)
  {}

  OrderInfo (const double price, const int shares)
    : _price (price)
    , _shares(shares)
    , _counter(0)
  {}

  double  _price;
  int     _shares;
  int     _counter;
};


class AroonPrint : public Element
{
  public:

    typedef HashMap<int, OrderInfo>         OrderMap;

    AroonPrint();
    ~AroonPrint();

    const char *class_name() const		{ return "AroonPrint"; }
    const char *port_count() const		{ return PORTS_1_1; }

    int configure(Vector<String> &, ErrorHandler *);
    bool can_live_reconfigure() const		{ return true; }
    void add_handlers();

    Packet *simple_action(Packet *);

  private:

    OrderMap        _orderMap;
    AroonIndicator  _aroonIndicator;

    String _label;
    int _bytes;		// How many bytes of a packet to print
    bool _active;
    bool _timestamp : 1;
    bool _headroom : 1;
#ifdef CLICK_LINUXMODULE
    bool _cpu : 1;
#endif
    bool _print_anno;
    uint8_t _contents;

};

CLICK_ENDDECLS
#endif
