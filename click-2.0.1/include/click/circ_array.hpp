// Copyright QUB 2017

#ifndef CircArrayH
#define CircArrayH

//#include <stddef.h>
//#include <stdio.h>
//#include <cstring>
//#include <cassert>

#include <click/config.h>
#include <click/glue.hh>

CLICK_DECLS

template<typename T>
class CircArray
{
public:
  enum RC
  {
    SUCCESS,
    NO_SPACE_ERROR,
    BUFFER_EMPTY_ERROR
  };
public:
  CircArray ();
  CircArray (const size_t capacity);
  ~CircArray ();

  CircArray (const CircArray& copy);
  CircArray& operator= (const CircArray& rhs);

  T& operator[](const size_t index) const;

  void    resize_by_two ();

  RC      push_front (const T&  value, const bool overwrite_when_full = false);
  RC      pop_front  (T&        value);
  T       pop_front  (); // use at your own risk - does assert

  T       pop_at     (const size_t index); // really better be using linked list instead of this

  RC      push_back  (const T&  value, const bool overwrite_when_full = false);
  RC      pop_back   (T&        value);
  T       pop_back   (); // use at your own risk - does assert

  size_t  occupancy  () { return _size; }

  size_t  capacity   () { return _capacity; }

  bool    is_full    () { return _size == _capacity; }

  bool    empty      () { return _size == 0; }

  void    print_contents      ();

private:
  void    set_initial_values  (const size_t capacity);

  void    deep_copy  (CircArray& lhs, const CircArray& rhs);

private:
  T*  _array;

  size_t _start;
  size_t _size;
  size_t _capacity;
}; // class CircArray

template<typename T>
CircArray<T>::CircArray ()
{
  set_initial_values (10);
}

template<typename T>
CircArray<T>::CircArray (const size_t capacity)
{
  set_initial_values (capacity);
}

template<typename T>
void CircArray<T>::set_initial_values (const size_t capacity)
{
  _array = new T[capacity];

  _start = _size = 0;

  _capacity = capacity;
}

template<typename T>
CircArray<T>::~CircArray ()
{
  if (_array)
  {
    delete _array;
  }
}

template<typename T>
CircArray<T>::CircArray (const CircArray& copy)
{
  _array    = NULL;
  _start    = _size = 0;
  _capacity = 0;

  deep_copy (*this, copy);
}

template<typename T>
CircArray<T>& CircArray<T>::operator= (const CircArray& rhs)
{
  deep_copy (*this, rhs);
  return *this;
}

template<typename T>
void CircArray<T>::deep_copy (CircArray& lhs, const CircArray& rhs)
{
  // if the arrays are of the same size, there is no need to
  // allocate/deallocate - we can just assign the vars
  if ((lhs._capacity == rhs._capacity) && lhs._array)
  {
    // first just copy the state
    lhs._start    = rhs._start;
    lhs._size     = rhs._size;
    lhs._capacity = rhs._capacity;

    // now copy contents
    for (int i = 0; i < rhs._size; ++i)
    {
      lhs[i] = rhs[i];
    }

    return;
  }

  if (lhs._array)
  {
    delete [] lhs._array;
  }

  lhs._start    = rhs._start;
  lhs._size     = rhs._size;
  lhs._capacity = rhs._capacity;

  lhs._array = new T[rhs._capacity];

  for (int i = 0; i < rhs._size; ++i)
  {
    lhs[i] = rhs[i];
  }
}

template<typename T>
void CircArray<T>::resize_by_two ()
{
  T* temp = new T[_capacity * 2];

  for (int i = 0; i < _size; ++i)
  {
    temp[i] = (*this)[i];
  } 

  // increase capacity, the _size remains the same
  _capacity *= 2;
  _start     = 0;

  delete [] _array;
  _array = temp;
}

template<typename T>
T& CircArray<T>::operator[] (const size_t index) const
{
  assert (index < _size);
  return _array[(_start + index) % _capacity];
}

// push front, pop front - adjusting just the size
template<typename T>
typename CircArray<T>::RC CircArray<T>::push_back (const T& value, const bool overwrite_when_full)
{
  if (_size == _capacity)
  {
    if (!overwrite_when_full)
    {
      return NO_SPACE_ERROR;
    }
    else
    {
      // FIXME: if size = capacity = 0, an exception will be thrown
      pop_front ();
      assert (_size < _capacity);
    }
  }

  _array[(_start + _size) % _capacity] = value;
  ++_size;
  return SUCCESS;
}

// FIXME
template<typename T>
typename CircArray<T>::RC CircArray<T>::pop_back (T& value)
{
  if (_size == 0)
  {
    return BUFFER_EMPTY_ERROR;
  }

  value = _array[(_start + _size - 1) % _capacity];
  --_size;

  return SUCCESS;
}

template<typename T>
T CircArray<T>::pop_back ()
{
  T value;
  RC rc = pop_back (value);

  assert (rc == SUCCESS);

  return value;
}
// ------------
template<typename T>
T CircArray<T>::pop_front ()
{
  T value;
  RC rc = pop_front (value);
  assert (rc == SUCCESS);
  return value;
}

// push back, pop back - adjusting the size and the start
template<typename T>
typename CircArray<T>::RC CircArray<T>::push_front (const T& value, const bool overwrite_when_full)
{
  if (_size == _capacity)
  {
    if (!overwrite_when_full)
    {
      return NO_SPACE_ERROR;
    }
    else
    {
      pop_back ();
      assert (_size < _capacity);
    }
  }

  if (_start == 0)
  {
    _start = _capacity - 1;
  }
  else
  {
    --_start;
  }
  _array[_start] = value;
  ++_size;

  return SUCCESS;
}

template<typename T>
typename CircArray<T>::RC CircArray<T>::pop_front (T& value)
{
  if (_size == 0)
  {
    return BUFFER_EMPTY_ERROR;
  }

  value = _array[_start];
  --_size;
  _start = (_start + 1) % _capacity;

  return SUCCESS;
}

template<typename T>
T CircArray<T>::pop_at (const size_t index)
{
  assert (index < _size);

  // (will be making a lot of copies)
  T element = (*this)[index];
  // TODO: an optimization would be to choose between
  // shifting to the beginning or the end depending
  // on the shortest path (i.e. the fewest elements
  // to sift through).
  // now shift all elements after index left
  for (int i = index; i < _size; ++i)
  {
    if (i == (_size - 1))
    {
      pop_back (); // just remove the last element
    }
    else
    {
      (*this)[i] = (*this)[i+1];
    }
  }

  return element;
}

template<typename T>
void CircArray<T>::print_contents()
{
  click_chatter ("Cannot print contents without template specialization\n");
}

template<>
void CircArray<int>::print_contents();

CLICK_ENDDECLS
#endif
