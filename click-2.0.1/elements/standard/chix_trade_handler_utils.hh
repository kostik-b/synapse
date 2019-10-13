// Copyright Queen's University Belfast 2012

#ifndef ChixTradeHandlerUtilsHH
#define ChixTradeHandlerUtilsHH

#include <click/fixedptc.h>

class ChixTradeHandlerUtils
{
public:

  static void eliminate_leading_whitespaces (
    unsigned char*&      field_ptr,
    int&                 remaining_len)
  {
    while ((*field_ptr == ' ') && (remaining_len > 0))
    {
      ++field_ptr;
      --remaining_len;
    }
  }

  template<const int PriceOffset,
           const int PriceLen,
           const int PriceWholeLen>
  static void parse_add_order_price (int&                 whole_part_int,
                                     int&                 decimal_part_int,
                                     const unsigned char* msg)
  {
    const int PriceDecimalLen = PriceLen - PriceWholeLen;

    char whole_part  [PriceLen];
    char decimal_part[PriceLen];
    memset (whole_part,    '\0', PriceLen);
    memset (decimal_part,  '\0', PriceLen);

    unsigned char* price_whole_ptr= const_cast<unsigned char*>(msg) + PriceOffset;
    int            remaining_len  = PriceWholeLen;

    eliminate_leading_whitespaces (price_whole_ptr, remaining_len);

    memcpy (whole_part, price_whole_ptr, remaining_len);
    whole_part_int = strtol (whole_part, NULL, 10);

    memcpy (decimal_part, msg + PriceOffset + PriceWholeLen, PriceDecimalLen);

    decimal_part_int     = strtol (decimal_part, NULL, 10);
  }

  template<const int PriceOffset,
           const int PriceLen,
           const int PriceWholeLen>
  static const double parse_add_order_price_as_double (const unsigned char* msg)
  {
    int whole_part_int    = 0;
    int decimal_part_int  = 0;
    parse_add_order_price<PriceOffset, PriceLen, PriceWholeLen> 
      (whole_part_int, decimal_part_int, msg);

    double  decimal_part_double  = double (decimal_part_int)/double(10000);

    return double(whole_part_int) + double(decimal_part_double);
  }

  template<const int PriceOffset,
           const int PriceLen,
           const int PriceWholeLen>
  static const fixedpt parse_add_order_price_as_fixedpt (const unsigned char* msg)
  {
    int whole_part_int    = 0;
    int decimal_part_int  = 0;
    parse_add_order_price<PriceOffset, PriceLen, PriceWholeLen> 
      (whole_part_int, decimal_part_int, msg);

    fixedpt decimal_part_float  = fixedpt_div (fixedpt_fromint (decimal_part_int), fixedpt_fromint (10000));

    return fixedpt_add(fixedpt_fromint(whole_part_int), decimal_part_float);
  }

  template <typename  NumericType,
            const int Offset,
            const int FieldLength>
  static NumericType parse_numeric (const unsigned char* msg)
  {
    // we need to eliminate the leading whitespaces - in kernel mode
    // this prevents the parsing of the function
    unsigned char* field_ptr      = const_cast<unsigned char*>(msg) + Offset;
    int            remaining_len  = FieldLength;

    eliminate_leading_whitespaces (field_ptr, remaining_len);

    char buffer [FieldLength + 1];
    memset (buffer, '\0', FieldLength + 1);

    memcpy (buffer, field_ptr, remaining_len);

    return strtol (buffer, NULL, 10);
  }

  template <const int FieldLen>
  static void trim_alphanumeric_spaces (
    char(& field)[FieldLen])
  {
    for (size_t i = 0; i < FieldLen; ++i)
    {
      if ((field[i] == ' ') && (i != 0))
      {
        field[i] = '\0';
      }
    }
  }

  static __inline__ uint64_t bmk_rdtsc( void )
  {
    uint64_t x;
    __asm__ volatile("rdtsc\n\t" : "=A" (x));
    return x;
  }


}; // class ChixTradeHandlerUtils

#endif
