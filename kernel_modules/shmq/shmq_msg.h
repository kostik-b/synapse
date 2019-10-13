// Copyright Queen's University of Belfast 2013

#ifndef SHMQ_MSG_HH
#define SHMQ_MSG_HH

#define SHMQ_MSG_MAX_DATA_LEN 120

static const int SHMQ_MSG_KERNEL_TYPE  = 1;
static const int SHMQ_MSG_USER_TYPE    = 2;

struct shmq_msg
{
  long int _mtype;
  struct
  {
    size_t  _size;
    char    _data[SHMQ_MSG_MAX_DATA_LEN];
  }_msg_data_struct;
  
}; // struct shmq_msg

#endif
