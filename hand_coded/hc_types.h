// Copyright QUB 2017

#ifndef hc_types_H
#define hc_types_H

#include <vector>
#include <string>

enum UpdateType
{
  ADD,
  UPDATE
};

struct Params
{
  Params ()
  {
    _is_trix = _is_dmi = _is_vortex = _is_adline = false;
    _indicator_debug = false;
    _ewma_periods = _interval_len_secs = _port = 0;
  }

  bool                      _is_trix;
  bool                      _is_dmi;
  bool                      _is_vortex;
  bool                      _is_adline;
  bool                      _indicator_debug;
  std::vector<std::string>  _symbols;
  int                       _ewma_periods;
  int                       _interval_len_secs;
  int                       _port;
}; // struct Params



#endif
