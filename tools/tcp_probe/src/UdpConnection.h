// Copyright QUB 2013

#ifndef UdpConnectionH
#define UdpConnectionH

#include "Connection.h"

#include <stdexcept>

using ProbeTool::ReturnValue;

class UdpConnection : public Connection
{
public:
  // this opens the connection
  UdpConnection (
    const char* ip_address,
    const int   port)
      : Connection (ip_address, port, SOCK_DGRAM, IPPROTO_UDP)
  { }

private:
  UdpConnection (); // no default constructor

}; // class UdpConnection

#endif
