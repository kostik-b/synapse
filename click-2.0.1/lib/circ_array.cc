// Copyright QUB 2017

#include <click/circ_array.hpp>
CLICK_DECLS

template<>
void CircArray<int>::print_contents()
{
  // need to populate the map of occupied cells
  char map [_capacity];
  memset (map, 'N', _capacity);
  for (int i = 0; i < _size; ++i)
  {
    map [(_start + i) % _capacity] = 'Y';
  }
  // first print header
  click_chatter ("+");
  for (int i = 0; i < _capacity; ++i)
  {
    click_chatter ("-%02d-", i);
  }
  click_chatter ("+\n");

  // not print contents
  click_chatter ("|");
  for (int i = 0; i < _capacity; ++i)
  {
    if (map[i] == 'N')
    {
      click_chatter ("nnn|");
    }
    else
    {
      click_chatter ("%03d|", _array[i]);
    }
  }

  click_chatter ("\n");

  // now print footer
  click_chatter ("+");
  for (int i = 0; i < _capacity; ++i)
  {
    click_chatter ("----");
  }
  click_chatter ("+\n");
}

CLICK_ENDDECLS
