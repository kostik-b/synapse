// Copyright Queen's University of Belfast 2012

#ifndef SYNAPSE_ARRAY_WRAPPER_H
#define SYNAPSE_ARRAY_WRAPPER_H

namespace Synapse
{

template<typename ArrayType, const int ArraySize>
class ArrayWrapper
{
public:
  // copy constructor
  ArrayWrapper ()
  {
    memset (_array, '\0', ArraySize);
  }

  ArrayWrapper (const ArrayType(&  array)[ArraySize])
  {
    memcpy (_array, array, ArraySize);
  }

  // copy constructor
  ArrayWrapper (const ArrayWrapper& copy)
  {
    memcpy (_array, copy._array, ArraySize);
  }

  ArrayWrapper& operator= (const char* str)
  {
    const int str_len = strlen (str);

    if (str_len >= ArraySize)
    {
      memset (_array, '\0', ArraySize);
    }
    else
    {
      memset (_array, '\0', ArraySize);
      memcpy (_array, str,  str_len);
    }

    return *this;
  }

  // assignment operator
  ArrayWrapper& operator= (const ArrayWrapper& rhs)
  {
    memcpy (_array, rhs._array, ArraySize);
  }

  bool operator==(ArrayWrapper& rhs)
  {
    return !memcmp (_array, rhs._array, ArraySize);
  }

  hashcode_t hashcode () const
  {
    return *((int32_t*)_array);
  }

  const ArrayType* array_ptr() const
  {
    return _array;
  }

  ArrayType _array[ArraySize];
}; // class ArrayWrapper

template<typename ArrayWrapper>
static bool operator== (const ArrayWrapper& lhs, const ArrayWrapper& rhs)
{
    return strcmp(lhs._array, rhs._array) == 0;
}

} // namespace Synapse

#endif
