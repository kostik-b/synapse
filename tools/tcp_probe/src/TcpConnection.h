// Copyright QUB 2013

#ifndef TcpConnectionH
#define TcpConnectionH

#include "Connection.h"

#include <stdexcept>

using ProbeTool::ReturnValue;

class TcpConnection : public Connection
{
public:
  // this opens the connection
  TcpConnection (
    const char* ip_address,
    const int   port)
     : Connection (ip_address, port, SOCK_STREAM, 0)
  { }


private:
  TcpConnection (); // no default constructor

}; // class TcpConnection

#endif
