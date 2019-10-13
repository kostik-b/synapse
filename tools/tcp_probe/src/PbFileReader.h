// Copyright QUB 2013

#ifndef PbFileReaderH
#define PbFileReaderH

#include <stdio.h>

static const int LINE_MAX_LEN = 200;

#define TEN_E6 10000000000ull

enum PbMode
{
  CHIX,
  KB
};

class PbFileReader
{
public:
  PbFileReader (const char* file_name,
                const bool  in_memory_file,
                PbMode      mode)
    : _current_line       (NULL)
    , _current_line_size  (0)
    , _next_line          (NULL)
    , _next_line_size     (0)
    , _interval           (0)
    , _mode               (mode)

#if 0
    , _last_line_ptr  (NULL)
    , _last_line_size (0)
    , _last_bytes_read(0)
    , _lines_in_memory(NULL)
    , _lines_in_file  (0)
#endif
  {
    _file_stream = fopen(file_name, "r");

    if (!_file_stream)
    {
      throw std::runtime_error ("PbFileReader: Could not open file specified");
    }

    _current_line       = static_cast<char*>(malloc (LINE_MAX_LEN));
    _current_line_size  = LINE_MAX_LEN;
    _next_line          = static_cast<char*>(malloc (LINE_MAX_LEN));
    _next_line_size     = LINE_MAX_LEN;

    if ((_current_line == NULL) || (_next_line == NULL))
    {
      throw std::runtime_error ("PbFileReader: could not malloc memory");
    }

    memset (_current_line, '\0', _current_line_size);
    memset (_next_line,    '\0', _next_line_size);

    advance (); // populate buffers

#if 0
    if (in_memory_file)
    {
      // allocate 10e6 lines
      printf ("Allocating 10 mln pointers ...\n");
      _lines_in_memory = new char* [TEN_E6];

      printf ("Allocating 10 mln buffers and reading the file\n");
      for (int i = 0; i < TEN_E6; i++)
      {
        _lines_in_memory[i] = new char [100];

        char*     line    = _lines_in_memory[i] + 4;
        ssize_t&  linelen = (ssize_t&)*(_lines_in_memory[i]);
        read_line (line, linelen);

        if (_lines_in_memory[i][4] == 0)  
        {
          _lines_in_file = i;
          break;
        }
      }
    }
#endif
  }

  ~PbFileReader ()
  {
    fclose (_file_stream);
  }

#if 0   
  bool read_line_from_memory (char*&    line_of_file,
                  ssize_t&  line_len)
  {
    static int counter = 0;

    if (counter > _lines_in_file)
    {
      return false;
    }

    line_of_file = _lines_in_memory [counter] + 4;
    line_len     = (ssize_t)*(_lines_in_memory [counter]);

    ++counter;
    return true;
  }
#endif

  bool read_line (char*&    line,
                  ssize_t&  line_len)
  {
    bool status = false;

    if ((_current_line[0] != '\0') && (_next_line[0] != '\0'))
    {
      line      = _current_line;
      line_len  = strlen (_current_line);

      status    = true;
    }
    else if ((_current_line[0] != '\0') && (_next_line[0] == '\0'))// no more lines to be read in
    {
      line      = _current_line;
      line_len  = strlen (_current_line);

      status    = false;
    }
    else
    {
      line      = NULL;
      line_len  = 0;

      status    = false;
    }

    if (line_len > 0)
    {
      --line_len; // to account for \n read in by the "getline" call
    }

    return status;
  }

  uint64_t get_interval ()
  {
    return _interval;
  }

  void advance ()
  {
    // 1. if current_line and next_line are empty, then fill them all
    if ((_current_line[0] == '\0') && (_next_line[0] == '\0'))
    {
      _interval = 0;
      int bytes = 0;

      while (_current_line[0] == '\0')
      {
        bytes = getline (&_current_line, &_current_line_size, _file_stream);
        if (bytes < 0)
        {
          break;
        }
      }

      while (_next_line[0] == '\0')
      {
        bytes = getline (&_next_line,    &_next_line_size,    _file_stream);
        if (bytes < 0)
        {
          break;
        }
      }
      _interval  = get_interval (_next_line, _current_line); // get_chix_tstamp (_next_line) - get_chix_tstamp (_current_line);
    }
    // 2. no more lines to be read in
    else if ((_current_line[0] != '\0') && (_next_line[0] == '\0'))
    {
      // do nothing?
    }
    // 3. else move next_line to current_line (through ptrs) and read next_line in
    else if ((_current_line[0] != '\0') && (_next_line[0] != '\0'))
    {
      char* temp    = _current_line;
      _current_line = _next_line;
      _next_line    = temp;

      size_t temp_size    = _current_line_size;
      _current_line_size  = _next_line_size;
      _next_line_size     = temp_size;

      _interval = 0;
      memset (_next_line, '\0', _next_line_size);
      
      do
      {
        int bytes = getline (&_next_line,    &_next_line_size,    _file_stream);
        if (bytes < 0)
        {
          break;
        }
      } while (_next_line[0] == '\0');

      _interval  = get_interval (_next_line, _current_line); // get_chix_tstamp (_next_line) - get_chix_tstamp (_current_line);
    }
  }

private:
  uint64_t get_interval (char* next_line, char* current_line)
  {
    if (_mode == CHIX)
    {
      // we calculate the delta right here
      return  uint64_t(get_chix_tstamp (_next_line) - get_chix_tstamp (_current_line)) * uint64_t(1000); // need to convert it from milliseconds to microseconds
    }
    else if (_mode = KB)
    {
      // in KB format the next line contains the delta relative to the
      // previous message
      return get_KB_tstamp (_next_line);
    }
  }

  int get_chix_tstamp (char* msg)
  {
    // first 8 bytes are a timestamp

    // save the 9th char and replace with null char
    char letter = msg [8];
    msg[8]      = '\0';

    // convert
    int  tstamp = atoi (msg);

    // restore the 9th char
    msg[8] = letter;

    return tstamp;
  }

  uint64_t get_KB_tstamp (char* msg)
  {
    printf ("KB: parsing string for timestamp: %s\n", msg);
    // it is the second field, i.e. R|TSTAMP|
    // find the first 2 vertical bars
    char* tstamp_start = NULL;
    int   bar_count    = 0;
    char* runner       = msg;

    while (runner != '\0')
    {
      if ((*runner == '|') && (bar_count == 0))
      {
        tstamp_start = runner + 1;
        ++bar_count;
      }
      if ((*runner == '|') && (bar_count == 1))
      {
        *runner = '\0';
        uint64_t ret_value = strtoll (tstamp_start, NULL, 10);
        *runner = '|';
        return ret_value;
      }
      
      ++runner;
    }
  }
#if 0
  // not thread-safe
  // would be good to implement
  // some sort of read-ahead mechanism
  // an extra thread would probably
  // be needed and circular buffer would
  // be handy
  void read_line (char*&    line_of_file,
                  ssize_t&  line_len)
  {
    // it reads including the new line
    // let the system allocate the buffer itself
    // it will get enlarged in future calls
    _last_bytes_read     = getline (&_last_line_ptr, &_last_line_size, _file_stream);
    // eos
    if (_last_bytes_read < 0)
    {
      line_of_file = NULL;
      line_len     = 0; // trim the newline
    }
    else
    {
      // TODO: put here the preprocessing techniques required
      line_of_file = _last_line_ptr;
      line_len     = _last_bytes_read - 1; // trim the newline
    }

  }
 
  void get_last_line (char*&    line_of_file,
                  ssize_t&  line_len)
  {
    line_of_file = _last_line_ptr;
    line_len     = _last_bytes_read - 1; // trim the newline
  }
#endif
private:
  char*     _current_line;
  size_t    _current_line_size;

  char*     _next_line;
  size_t    _next_line_size;

  uint64_t  _interval; // delta in microseconds

#if 0
  char*   _last_line_ptr;
  size_t  _last_line_size;
  ssize_t _last_bytes_read;
#endif 
  FILE*     _file_stream;
  PbMode    _mode;

#if 0
  char**  _lines_in_memory;
  int     _lines_in_file;
#endif
private:
  PbFileReader ();

  PbFileReader (const PbFileReader& copy);
}; // class PbFileReader

#endif
