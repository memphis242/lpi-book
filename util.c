#include <stdint.h>
#include <string.h>

#include <assert.h>

int
format_num2str( int64_t num,
                char * const buf,
                size_t sz )
{
   if ( buf == nullptr || sz == 0 )
      return -1;

   // Determine number of characters needed...
   size_t num_chars_needed;
   if      ( num < 10 )
      num_chars_needed = 1;
   else if ( num < 100 )
      num_chars_needed = 2;
   else if ( num < 1'000 )
      num_chars_needed = 3;
   else if ( num < 10'000 )
      num_chars_needed = 5; // account for commas
   else if ( num < 100'000 )
      num_chars_needed = 6;
   else if ( num < 1'000'000 )
      num_chars_needed = 7;
   else if ( num < 10'000'000 )
      num_chars_needed = 9;
   else if ( num < 100'000'000 )
      num_chars_needed = 10;
   else if ( num < 1'000'000'000 )
      num_chars_needed = 11;
   else if ( num < 10'000'000'000 )
      num_chars_needed = 13;
   else if ( num < 100'000'000'000 )
      num_chars_needed = 14;
   else if ( num < 1'000'000'000'000 )
      num_chars_needed = 15;
   else if ( num < 10'000'000'000'000 )
      num_chars_needed = 17;
   else if ( num < 100'000'000'000'000 )
      num_chars_needed = 18;
   else if ( num < 1'000'000'000'000'000 )
      num_chars_needed = 19;
   else if ( num < 10'000'000'000'000'000 )
      num_chars_needed = 21;
   else if ( num < 100'000'000'000'000'000 )
      num_chars_needed = 22;
   else if ( num < 1'000'000'000'000'000'000 )
      num_chars_needed = 23;
   else
      num_chars_needed = 25;

   // Every 4th char is a comma, so need one more or one less than that point.
   assert(num_chars_needed % 4 != 0);

   if ( sz < num_chars_needed )
      return -2;

   for ( int8_t i = (num_chars_needed-1), j=0; i >= 0; --i, ++j )
   {
      assert(j <= (int8_t)num_chars_needed);

      if ( ((j+1) % 4 == 0)
           && j > 2
           && (size_t)j < num_chars_needed )
      {
         buf[i] = ',';
         continue;
      }

      uint8_t digit = num % 10; // extract least-significant digit
      num /= 10; // divide away last digit
      buf[i] = (char)digit + '0';
   }

   assert(num == 0);

   return 0;
}
