// Copyright QUB 2018

#include <stdio.h>
#include "fixedpt_cpp.h"

int main (int argc, char** argv)
{
  // Just testing some features of the FixedPt class
  printf ("a = 4, b = 9, c = -6\n");

  // 1. subtraction down to negative
  FixedPt a = FixedPt::fromInt (4);
  FixedPt b = FixedPt::fromInt (9);

  printf ("1. a - b is %s\n", (a-b).c_str());
  // 2. division of negative by negative (should be positive at the end)
  FixedPt c = FixedPt::fromInt (-6);
  printf ("2. (a-b)/c is %s\n", ((a-b)/c).c_str());
  // 3. comparison operators (both of them)
  printf ("3a. a>b is %d\n", a>b);
  printf ("3b. a<b is %d\n", a<b);
  // 4. addition with integers
  printf ("4a. 2+a is %s\n", (2 + a).c_str());
  printf ("4b. 2-b is %s\n", (2 - b).c_str());
  // 5. abs functions
  printf ("5a. abs(6-a) is %s\n", abs (6 - a).c_str());
  printf ("5b. abs(c) is %s\n", abs (c).c_str());
  printf ("5c. abs(a) is %s\n", abs (a).c_str());

  return 0;
}
