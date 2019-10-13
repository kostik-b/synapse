
#ifndef SYNAPSE_MACROS_H
#define SYNAPSE_MACROS_H

#ifdef CLICK_LINUXMODULE
  #include <click/cxxprotect.h>
  CLICK_CXX_PROTECT
  #include <asm/i387.h>
  CLICK_CXX_UNPROTECT
  #include <click/cxxunprotect.h>

  #define FPU_START() kernel_fpu_begin ()
  #define FPU_END()   kernel_fpu_end ()

#else
  #define FPU_START()
  #define FPU_END()

#endif

// I'll introduce a build option later, for now
// we simply set it here
#define SYNAPSE_LATENCY_MONITOR 1

#ifdef SYNAPSE_LATENCY_MONITOR
  #define REPORT_LATENCY(md_msg) report_cycles(md_msg)
#else
  #define REPORT_LATENCY(md_msg)
#endif

#endif

