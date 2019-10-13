#ifndef CLICK_NDM_KB_HH
#define CLICK_NDM_KB_HH
#include <click/element.hh>
#include <click/string.hh>

#include <click/global_sizes.hh>
#include "indicator_base.hh"
#include "synapseelement.hh"

CLICK_DECLS

class Ndm : public IndicatorBase
{
  public:
    Ndm();
    ~Ndm();

    const char *class_name() const		{ return "Ndm"; }

    int configure(Vector<String> &, ErrorHandler *);

    // takes in a buffer, returns 1 value
    virtual FixedPt       process_naive       (Buffers&         buffers);
    // takes in a buffer, returns 1 value + cache
    virtual VectorCache&  process_ext         (Buffers&         buffers);
    // takes in an increment + cache, returnes 1 value + cache
    virtual VectorCache&  process_opt_ext     (Vector<FixedPt>& increments,
                                               VectorCache&     cache);

    virtual FixedPt     initialize_element    (Buffers&         buffers);

  public:
    VectorCache _local_cache;

}; // class Ndm

CLICK_ENDDECLS
#endif
