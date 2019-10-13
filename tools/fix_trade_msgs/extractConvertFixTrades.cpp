// Copyright QUB 2017

#include <stdio.h>
#include "hffix.hpp"
#include <time.h>
#include <stdint.h>

static uint64_t getTimeMicrosecs (const std::string& timestamp)
{
  tm tstampTm;

  char* rc = strptime (timestamp.c_str(), "%Y%m%d%H%M%S", &tstampTm);

  if (!rc)
  {
    printf ("KB: disaster!!\n");
    return 0;
  }

//  printf ("KB: Parsed string %s\n", timestamp.c_str());

/*
  printf ("The struct is %d-%d-%d::%d:%d:%d\n", tstampTm.tm_year, tstampTm.tm_mon, tstampTm.tm_mday,
                                                tstampTm.tm_hour, tstampTm.tm_min, tstampTm.tm_sec);
*/
  uint64_t microsecs = uint64_t(timegm (&tstampTm)) * uint64_t(1000000);

  // extract the microseconds part from the string
  char buffer [7];
  memcpy (buffer, timestamp.c_str() + 14, 6);
  buffer[6] = '\0';
  int extract = atoi (buffer);
//  printf ("The extract is %d\n", extract);

  microsecs += extract;

//  printf ("mircosecs is %lld\n", microsecs);
  return microsecs;
}

static void printMdEntry (hffix::message_reader::const_iterator group_begin,
                          hffix::message_reader::const_iterator group_end,
                          const std::string&                    timestamp)
{
  // convert timne to time_t
  uint64_t microsecsSinceEpoch = getTimeMicrosecs (timestamp);
  static uint64_t prevtstamp   = microsecsSinceEpoch;

  printf ("KB: microsecs diff is %lld\n", microsecsSinceEpoch - prevtstamp);

  fprintf (stderr, "R|%lld", microsecsSinceEpoch - prevtstamp);
  // get symbol
  hffix::message_reader::const_iterator j = std::find_if(group_begin, group_end, hffix::tag_equal(hffix::tag::Symbol));
  if (j != group_end)
  {
    printf ("KB: symbol is %s\n", j->value().as_string().c_str());
    fprintf (stderr, "|%s", j->value().as_string().c_str());
  }
  // get price
  j = std::find_if(group_begin, group_end, hffix::tag_equal(hffix::tag::MDEntryPx));
  if (j != group_end)
  {
    printf ("KB: price is %s\n", j->value().as_string().c_str());
    fprintf (stderr, "|%s", j->value().as_string().c_str());
  }
  // get size
  j = std::find_if(group_begin, group_end, hffix::tag_equal(hffix::tag::MDEntrySize));
  if (j != group_end)
  {
    printf ("KB: size is %s\n", j->value().as_string().c_str());
    fprintf (stderr, "|%s\n", j->value().as_string().c_str());
  }

  prevtstamp = microsecsSinceEpoch;
}

int main (int argc, char** argv)
{
  if (argc != 2)
  {
    printf ("No args\n");
    return 0;
  }
  printf ("KB: file name is %s\n", argv[1]);

  size_t buf_size = 2048;
  char* buffer = static_cast<char*>(malloc (buf_size));
  // open the file
  FILE* file = fopen (argv[1], "r");
  // read a line
  while (true)
  {

    ssize_t rc = getline (&buffer, &buf_size, file);
    // parse it with quickfix
    if (rc < 0)
    {
      break;
    }
 
    hffix::message_reader reader (buffer, rc - 1);

    hffix::message_reader::const_iterator iter = reader.begin ();
    
    printf ("--------\n");

    std::string timestamp;
    if (reader.find_with_hint (hffix::tag::SendingTime, iter))
    {
      timestamp = iter->value().as_string();
    }

    hffix::message_reader::const_iterator group_begin = std::find_if(reader.begin(), reader.end(), hffix::tag_equal(hffix::tag::MDUpdateAction));

    if (group_begin != reader.end())
    { // There is at least one group

      static int msgCounter = 0;
      printf ("KB: parsing message %d\n", msgCounter++);
      hffix::message_reader::const_iterator group_end   = std::find_if(group_begin + 1, reader.end(), hffix::tag_equal(hffix::tag::MDUpdateAction)); // set the group end

      do
      {
        // this is where I extract all the data
        if (group_begin->value ().as_string() == "0") // MDUpdateAction == new
        {
          hffix::message_reader::const_iterator j = std::find_if(group_begin, group_end, hffix::tag_equal(hffix::tag::MDEntryType));
          if ((j != group_end) && (j->value().as_string() == "2")) // trade
          {
            printMdEntry (group_begin, group_end, timestamp);
          }
        }
        // now try to advance the group_begin and group_end
        if (group_end == reader.end ())
        {
          break; // finish line
        }
        else
        {
          group_begin = group_end;
          group_end = std::find_if(group_begin + 1, reader.end(), hffix::tag_equal(hffix::tag::MDUpdateAction));// increment
        }
      } while (true);
    }
  }
  // close the file
  fclose (file);
}
