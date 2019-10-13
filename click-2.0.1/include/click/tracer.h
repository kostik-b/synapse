// Copyright QUB 2018

#ifndef synapse_tracer_h
#define synapse_tracer_h

#include <cstdio>
#include <cstring>
#include <map>
#include "fixedpt_cpp.h"
#include <cassert>
#include <string>

class Tracer
{
  Tracer ()
  {
    const char* file_name = getenv ("TRACER_NAME");
    assert (file_name != NULL);
    _fd   = fopen (file_name, "w");
    _type = "NO_TYPE";
  }

  ~Tracer ()
  {
    fclose (_fd);
  }
public:

  static Tracer& get_instance ()
  {
    static Tracer* tracer = new Tracer;

    return *tracer;
  }

  void reset_values ()
  {
    // _high = _low = _close = _pdm = _ndm = _tr = _pdi = _ndi = _dx = _vmu = _vmd = _viu = _vid = FixedPt::fromInt (0);
    _map.clear ();
  }

  void add_value (const char* class_name, const FixedPt& value)
  {
    // ignore the below elements
    if (strcmp (class_name, "Sum") == 0)
    {
      return;
    }
    std::pair<std::map<std::string, FixedPt>::iterator, bool> rc =  _map.insert (std::make_pair (class_name, value));

    if (rc.second == false) // the value already there
    {
      _map.erase (rc.first); // delete the previous value
      _map.insert (std::make_pair (class_name, value)); // ... and insert the new one again
    }
#if 0
    if (strcmp (class_name, "High") == 0)
    {
      _high = value;
    }
    else if (strcmp (class_name, "Low") == 0)
    {
      _low = value;
    }
    else if (strcmp (class_name, "Close") == 0)
    {
      _close = value;
    }
    else if (strcmp (class_name, "Pdm") == 0)
    {
      _pdm = value;
    }
    else if (strcmp (class_name, "Ndm") == 0)
    {
      _ndm = value;
    }
    else if (strcmp (class_name, "NewTrueRange") == 0)
    {
      _tr = value;
    }
    else if (strcmp (class_name, "Pdi") == 0)
    {
      _pdi = value;
    }
    else if (strcmp (class_name, "Ndi") == 0)
    {
      _ndi = value;
    }
    else if (strcmp (class_name, "Vmu") == 0)
    {
      _vmu = value;
    }
    else if (strcmp (class_name, "Vmd") == 0)
    {
      _vmd = value;
    }
    else if (strcmp (class_name, "Viu") == 0)
    {
      _viu = value;
    }
#endif
    if (strcmp (class_name, "Dx") == 0)
    {
//      _dx = value;
      print_all ();

      _type = "NO_TYPE";
      reset_values ();
    }
    else if (strcmp (class_name, "Vid") == 0)
    {
//      _vid = value;
      print_all ();

      _type = "NO_TYPE";
      reset_values ();
    }

  }

  void add_type (const char* type)
  {
    _type = type;
  }

  void print_all ()
  {
#if 0
    fprintf (_fd, "+--------+\n");
    fprintf (_fd, "Type is %s\n", _type);

    fprintf (_fd, "High   -> %s\n", _high.c_str());
    fprintf (_fd, "Low    -> %s\n", _low.c_str());
    fprintf (_fd, "Close  -> %s\n", _close.c_str());

    fprintf (_fd, "Pdm    -> %s\n", _pdm.c_str());
    fprintf (_fd, "Ndm    -> %s\n", _ndm.c_str());
    fprintf (_fd, "Tr     -> %s\n", _tr.c_str());
    fprintf (_fd, "Pdi    -> %s\n", _pdi.c_str());
    fprintf (_fd, "Ndi    -> %s\n", _ndi.c_str());
    fprintf (_fd, "Dx     -> %s\n", _dx.c_str());

    fprintf (_fd, "Vmu    -> %s\n", _vmu.c_str());
    fprintf (_fd, "Vmd    -> %s\n", _vmd.c_str());
    fprintf (_fd, "Viu    -> %s\n", _viu.c_str());
    fprintf (_fd, "Vid    -> %s\n", _vid.c_str());
#endif
    std::map<std::string, FixedPt>::iterator iter = _map.begin ();
    fprintf (_fd, "+--------+\n");
    fprintf (_fd, "Type is %s\n", _type);
    for ( ; iter != _map.end (); ++iter)
    {
      fprintf (_fd, "%s -> %s\n", iter->first.c_str (), iter->second.c_str());
    }
  }

private:
/*
// common
   FixedPt _high;
   FixedPt _low;
   FixedPt _close;
// dmi
   FixedPt _pdm;
   FixedPt _ndm;
   FixedPt _tr;
   FixedPt _pdi;
   FixedPt _ndi;
   FixedPt _dx;
// vortex
  FixedPt _vmu;
  FixedPt _vmd;
  FixedPt _viu;
  FixedPt _vid;
*/  
  std::map<std::string, FixedPt>  _map;
  FILE*                           _fd;
  const char*                     _type;
}; // class Tracer

#endif
