
#include <click/config.h>
#include "unique_tee.hh"
#include <click/args.hh>
#include <click/error.hh>
CLICK_DECLS

UniqueTee::UniqueTee()
{
}

UniqueTee::~UniqueTee()
{
}

int
UniqueTee::configure(Vector<String> &conf, ErrorHandler *errh)
{
    unsigned n = noutputs();
    if (Args(conf, this, errh).read_p("N", n).complete() < 0)
	return -1;
    if (n != (unsigned) noutputs())
	return errh->error("%d outputs implies %d arms", noutputs(), noutputs());
    return 0;
}

void
UniqueTee::push(int, Packet *p)
{
  int num_outputs = noutputs();

  for (int i = 0; i < num_outputs - 1; i++)
  {
    if (Packet *q = p->clone())
    {
      WritablePacket* write_packet = q->uniqueify ();
      output(i).push(write_packet);
    }
  }

  // the original packet is the last one to be pushed
  output(num_outputs - 1).push(p);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(UniqueTee)
