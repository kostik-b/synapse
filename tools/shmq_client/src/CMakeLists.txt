
PROJECT (SHMQ_CLIENT)

ADD_EXECUTABLE (shmq_client_pcap  shmq_client_pcap.c)

TARGET_LINK_LIBRARIES (shmq_client_pcap ${CMAKE_CURRENT_SOURCE_DIR}/libpcap.a)
#TARGET_LINK_LIBRARIES (shmq_client pcap)

ADD_EXECUTABLE (shmq_client_perf shmq_client_perf.cpp cycles_counter.cc)

SET_TARGET_PROPERTIES (shmq_client_perf PROPERTIES COMPILE_FLAGS -g)
SET_TARGET_PROPERTIES (shmq_client_pcap PROPERTIES COMPILE_FLAGS -g)
