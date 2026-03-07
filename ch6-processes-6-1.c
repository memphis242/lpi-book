#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <assert.h>

#include "util.h"

char globBuf[65536]; // Uninitialized data segment
int primes[] = { 2, 3, 5, 7 }; // Initialized data segment

static int
squares(int x) // x allocated in stack frame for squares()
{
   long int result; // result allocated in stack frame for squares()

   result = (long int)x * x;
   if ( result > INT_MAX )
      result = -1;
   else if ( result < x ) // overflow detection
      result = -2;

   return (int)result; // return value passed via register, or some other ABI
}

static int
do_calc(int val) // val allocated in stack frame for do_calc()
{
   // 26 characters, including commas, is as much as we'll need for numbers
   // up to 2^63.
   char val_str   [26] = {0};
   char square_str[26] = {0};
   char cube_str  [26] = {0};
   int rc;

   rc = format_num2str(val, val_str, sizeof val_str);
   assert(rc == 0);

   // Arguments to squares() placed in its stack frame, or some other ABI
   int square = squares(val); // square allocated in stack frame for do_calc()
   if ( square == -1 || square == -2 )
      return square; // return value passed via register, or some other ABI

   rc = format_num2str(square, square_str, sizeof square_str);
   assert(rc == 0); // we should not have mis-called this lib fcn, by design

   // Arguments to printf() placed in its stack frame, or some other ABI
   (void)printf( "The square of %s is %s\n",
                 val_str, square_str );

   // cube allocated in stack frame for do_calc()
   long int cube = (long int)square * val;
   if ( cube > LONG_MAX )
      return -3; // return value passed via register, or some other ABI
   else if ( cube < square ) // overflow detection
      return -4; // return value passed via register, or some other ABI

   rc = format_num2str(cube, cube_str, sizeof cube_str);
   assert(rc == 0); // we should not have mis-called this lib fcn, by design

   // Arguments to printf() placed in its stack frame, or some other ABI
   (void)printf( "The cube of %s is %s\n",
                 val_str, cube_str );

   rc = format_num2str(INT64_MAX, val_str, sizeof val_str);
   assert(rc == 0);
   (void)printf("INT64_MAX: %s\n", val_str);

   return 0; // return value passed via register, or some other ABI
}

int
main(int argc, char *argv[]) // stack frame of main()
{
   static int key = 9947; // initialized data segment
   static char mbuf[10'240'000]; // uninitialized data segment

   char *p; // stack frame of main()
   p = malloc(1024); // points to block allocated on the heap, assuming success

   if ( p == nullptr )
      return -5;

   // stack frame of of main() /w args passed to stack frame of do_calc() or other ABI
   int rc = do_calc(key);

   return rc;
}
