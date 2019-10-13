/*
 * tee.{cc,hh} -- element duplicates packets
 * Eddie Kohler
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include "reuse_tee.hh"
#include <click/args.hh>
#include <click/error.hh>
#include <click/appmsgs.hh>

using Synapse::MsgValue;

CLICK_DECLS

ReuseTee::ReuseTee()
{
}

ReuseTee::~ReuseTee()
{
}

int
ReuseTee::configure(Vector<String> &conf, ErrorHandler *errh)
{
    unsigned n = noutputs();
    if (Args(conf, this, errh).read_p("N", n).complete() < 0)
	return -1;
    if (n != (unsigned) noutputs())
	return errh->error("%d outputs implies %d arms", noutputs(), noutputs());
    return 0;
}

void
ReuseTee::push(int, Packet *p)
{
  assert (p->length () < COPY_MSG_MAX_LEN);

  // the idea here is similar to that of SourceSplit
  memset (_copy_buffer, '\0', COPY_MSG_MAX_LEN);
  memcpy (_copy_buffer, p->data(), p->length ());

  const size_t  copy_len  = p->length ();
  PacketAppType type_copy = p->get_packet_app_type ();

  int n = noutputs();
  for (int i = 0; i < n - 1; i++)
  {
    if (Packet *q = p->clone())
    {
      assert (q->length () < COPY_MSG_MAX_LEN); 

      WritablePacket* write_packet = q->put (0);
      memcpy (write_packet->data(), _copy_buffer, copy_len);
      write_packet->set_packet_app_type (type_copy);

      output(i).push(write_packet);
    }
  }

  assert (p->length () < COPY_MSG_MAX_LEN); 

  WritablePacket* write_packet = p->put (0);
  memcpy (write_packet->data(), _copy_buffer, copy_len);
  write_packet->set_packet_app_type (type_copy);

  output(n - 1).push(write_packet);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(ReuseTee)
ELEMENT_MT_SAFE(ReuseTee)
