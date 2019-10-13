// Copyright Queen's University Belfast 2013

#ifndef CLICK_UTILS_HH
#define CLICK_UTILS_HH

namespace Utils
{

static __inline__ uint64_t bmk_rdtsc( void )
{
  uint64_t x;
  __asm__ volatile("rdtsc\n\t" : "=A" (x));
  return x;
}

} // namespace Utils
#endif
