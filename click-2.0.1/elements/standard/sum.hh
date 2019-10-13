#ifndef CLICK_SUM_KB_HH
#define CLICK_SUM_KB_HH
#include <click/element.hh>
#include <click/string.hh>

#include <click/global_sizes.hh>
#include "indicator_base.hh"
#include "synapseelement.hh"

CLICK_DECLS

class Sum : public IndicatorBase
{
  public:
    Sum();
    ~Sum();

    const char *class_name() const		{ return "Sum"; }

    int configure(Vector<String> &, ErrorHandler *);

    // takes in a buffer, returns 1 value
    virtual FixedPt       process_naive       (Buffers&         buffers);
    // takes in a buffer, returns 1 value + cache
    virtual VectorCache&  process_ext         (Buffers&         buffers);
    // takes in an increment + cache, returnes 1 value + cache
    virtual VectorCache&  process_opt_ext     (Vector<FixedPt>& increments,
                                               VectorCache&     cache);

    virtual FixedPt     initialize_element    (Buffers&         buffers);


  private:
    int           _sum_periods;

    VectorCache   _local_cache;
}; // class Sum

CLICK_ENDDECLS
#endif
