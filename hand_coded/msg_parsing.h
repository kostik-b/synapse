// Copyright QUB 2017

#ifndef msg_parsing_h
#define msg_parsing_h

#include "fixedptc.h"
#include "msg_trade.h"

static const size_t s_whole_price_len       = 10;
static const size_t s_fractional_price_len  = 10;

static int denominator_for_len (size_t len)
{
  switch (len)
  {
    case 1:
      return 10;
      break;
    case 2:
      return 100;
      break;
    case 3:
      return 1000;
      break;
    case 4:
      return 10000;
      break;
    case 5:
      return 100000;
      break;
    case 6:
      return 1000000;
      break;
  }
}

static fixedpt parse_price_as_fixedpt (
  const char*   str,
  const size_t  strlen)
{
  const char* dot_ptr = NULL;
  for (int i = 0; i < strlen; ++i)
  {
    if (str[i] == '.')
    {
      dot_ptr = str + i;
      break;
    }
  }

  // convert the whole and decimal parts now
  if (dot_ptr == NULL) // only whole part
  {
    char buffer [strlen + 1];
    memcpy (buffer, str, strlen);
    buffer[strlen] = '\0';
    return fixedpt_fromint (strtol (buffer, NULL, 10));
  }
  else if (dot_ptr == str + 1) // only fractional part
  {
    char buffer [strlen];
    memcpy (buffer, str + 1, strlen - 1);
    buffer[strlen - 1] = '\0';

    fixedpt price = 0;
    
    long    fr    = strtol (buffer, NULL, 10);
    // divide the fractional part by 10^(fr part len), e.g. 23/100 or 435/1000. Then add to the price
    fixedpt_add (price, fixedpt_div (fixedpt_fromint (fr), 
                                     fixedpt_pow(FIXEDPT_ONE, strlen - 1)));
    return price;
  }
  else
  {
    char whole_part       [s_whole_price_len];
    char fractional_part  [s_fractional_price_len];

    memset (whole_part,     '\0', s_whole_price_len);
    memset (fractional_part,'\0', s_fractional_price_len);

    size_t whole_part_len       = dot_ptr - str;
    size_t fractional_part_len  = strlen - whole_part_len - 1;

    strncpy (whole_part,      str,          whole_part_len);
    strncpy (fractional_part, dot_ptr + 1,  fractional_part_len);

    // divide the fractional part by 10^(fr part len), e.g. 23/100 or 435/1000. Then add to the price
    // whole part is easy
    fixedpt price = fixedpt_fromint (strtol (whole_part, NULL, 10));
    // convert to int
    long    fr                          = strtol (fractional_part, NULL, 10);
    fixedpt frac_fixedpt = fixedpt_div (fixedpt_fromint (fr), 
                                     fixedpt_fromint (denominator_for_len (fractional_part_len)));

    
    price = fixedpt_add (price, frac_fixedpt);

    return price;
  }
}

static bool populate_msg_trade (
  MsgTrade&     msg,
  const char*   packet,
  const size_t  packet_len)
{
  // how do we filter out the wrong messages?
  // -> probably by simply parsing the msg bit by by bit

  const unsigned  tstamp_len      = 8;
  const char*     md_msg          = packet + tstamp_len;
  const uint32_t  md_len          = packet_len - tstamp_len;

  int                   component_start = 0;
  int                   counter         = 0;

  for (int i = 0; i < md_len; ++i)
  {
    if (md_msg[i] == '|')
    {
      ++counter;

      switch (counter)
      {
        case 0: // should never happen
        case 1: // do nothing 
        case 2: // it's the timestamp delta from previous message - ignore
          break;
        case 3: // it's a symbol
          {
            size_t symbol_len = i - component_start;

            if (symbol_len > ORDER_SYMBOL_LEN - 1)
            {
              fprintf (stderr, "ERROR: Symbol is too long!\n");
              return false;
            }
            strncpy (msg._symbol, md_msg + component_start, symbol_len);
          }
          break;
        case 4: // it's a price, the size is immediately after
          {
            size_t price_len = i - component_start;
            msg._price = parse_price_as_fixedpt (md_msg + component_start, price_len);
            if ((i + 1) >= md_len)
            {
              fprintf (stderr, "No size detected in the trade msg!!!\n");
              return false;
            }
            // now parse the size
            const char*   size_ptr = md_msg + i + 1;
            size_t        size_len = md_len - i - 1;

            char buffer [size_len + 1];
            memcpy (buffer, size_ptr, size_len);
            buffer [size_len] = '\0';

            msg._size = strtol (buffer, NULL, 10);
          }
          break;
        default:
          // too many vertical bars - return false
          return false;
          break;
      }

      component_start = i + 1; // the next char after the vertical bar
    }
  }

  if (counter < 1)
  {
    return false; // incorrect message basically - no delimiters
  }

  // this timestamp is produced by timestamper - if we get here, the msg is the correct one
  msg._src_timestamp = *(reinterpret_cast<const uint64_t*>(packet));
  //click_chatter ("KB: timestamp is %lld", msg._src_timestamp);
#if 0
  if (false)
  {
    click_chatter ("TP: populated MsgTrade");
    click_chatter ("TP: the packet was:");
    print_as_hex_inline (md_msg, md_len);
    click_chatter ("TP: the trade msg is: symbol - %s, price - %s, size - %d",
                    msg._symbol, fixedpt_cstr (msg._price, 4), msg._size);
  }
#endif

  return true;
}



#endif
