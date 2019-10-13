#ifndef CLICK_NDI_KB_HH
#define CLICK_NDI_KB_HH
#include <click/element.hh>
#include <click/string.hh>

#include <click/global_sizes.hh>
#include "indicator_base.hh"
#include "synapseelement.hh"

CLICK_DECLS

class Ndi : public IndicatorBase
{
  public:
    Ndi();
    ~Ndi();

    const char *class_name() const		{ return "Ndi"; }

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
    VectorCache _local_cache;

}; // class Ndi

CLICK_ENDDECLS
#endif
