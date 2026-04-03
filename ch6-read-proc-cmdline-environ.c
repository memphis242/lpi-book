#define _POSIX_C_SOURCE 200809L // Specify atleast POSIX.1-2008 compatibility
#define _GNU_SOURCE

// Standard C Headers
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
// System Headers
#include <unistd.h>
#include <sys/types.h>

#if !defined(__GLIBC__) || !__GLIBC_PREREQ(2, 23)
#define strerrorname_np(str) "strerrorname_np() NOT IMPLEMENTED"
#endif

#define ARR_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

//extern char ** environ;

int main(int argc, char * argv[])
{
   int rc = 0;

   pid_t my_pid = getpid();

   assert(my_pid > 1);
   assert((unsigned long)my_pid <= LONG_MAX);

   // Read /proc/[PID]/cmdline, print out contents, then print argv and compare.

   // First, open the file
   char cmdline_fname[ sizeof("/proc/") + 8 + sizeof("/cmdline") - 2 + 1 ] = {0};
   int nbytes = snprintf(cmdline_fname, sizeof cmdline_fname,
                         "/proc/%d/cmdline",
                         my_pid);

   static_assert(sizeof cmdline_fname < INT_MAX);
   assert(nbytes < (int)(sizeof cmdline_fname));

   (void)printf("Reading and parsing %s...\n\n", cmdline_fname);

   FILE *fp = fopen(cmdline_fname, "r");
   if ( fp == nullptr )
   {
      (void)fprintf(stderr,
               "Error: fopen(\"%s\", \"r\") failed\n"
               "Returned nullptr :: errno: %s (%d) :: %s\n",
               cmdline_fname,
               strerrorname_np(errno), errno, strerror(errno) );

      return 1;
   }

   // Then read file contents
   char cmdline_buf[1024] = {0}; // I'm not expecting more than that for this test program
   nbytes = 0;
   int c;
   while ( (c = fgetc(fp)) != EOF && nbytes < (int)(sizeof cmdline_buf) )
      cmdline_buf[nbytes++] = (char)c;

   if ( nbytes >= (int)(sizeof cmdline_buf) )
   {
      (void)fprintf(stderr,
               "Error: cmdline file too large.\n"
               "       Program only written to support up to %zu bytes in cmdline file.",
               sizeof cmdline_buf );

      (void)fclose(fp);
      return 2;
   }

   // Then split out cmdline arguments into argv-like array
   assert(nbytes <= (int)(sizeof cmdline_buf));

   char cmdline_args[8][128] = {0};
   char prev_char = '\0';

   static_assert(sizeof cmdline_args == sizeof cmdline_buf);

   size_t nargs=0, arg_int_idx=0;
   for ( size_t fbuf_idx=0;
         fbuf_idx < (size_t)nbytes && nargs < ARR_LEN(cmdline_args) && arg_int_idx < ARR_LEN(cmdline_args[0]);
         ++fbuf_idx )
   {
      char c = cmdline_buf[fbuf_idx];
      cmdline_args[nargs][arg_int_idx] = c;

      if ( c == '\0' )
      {
         ++nargs;
         arg_int_idx = 0;

         // Shouldn't have two nul characters in a row or as the first character...
         assert(prev_char != '\0');
      }
      else
      {
         ++arg_int_idx;
      }

      prev_char = c;
   }

   assert(nargs <= ARR_LEN(cmdline_args));
   assert(arg_int_idx <= ARR_LEN(cmdline_args[0]));
#ifndef NDEBUG
   for ( size_t i=0; i < nargs; ++i )
      assert(strnlen(cmdline_args[i], ARR_LEN(cmdline_args[0])) > 0);
#endif

   // Now print out what we read, alongside argv, and also compare simultaneously.
   assert(argc >= 0);
   assert((size_t)argc == nargs);
   for ( size_t i=0; i < nargs; ++i )
   {
      (void)printf("cmdline_args[%zu]: %s\n"
                   "        argv[%zu]: %s\n",
                   i, cmdline_args[i],
                   i, argv[i] );

      bool args_equivalent = strcmp((char *)cmdline_args[i], argv[i]) == 0;
      if ( !args_equivalent )
         puts("Mismatch!");

      (void)puts("");

      (void)fflush(stdout);
   }

   // Now close cmdline file.
   rc = fclose(fp);
   if ( rc != 0 )
   {
      (void)fprintf(stderr,
               "Error: Unable to fclose() cmdline file.\n"
               "       fclose() returned: %d :: errno %s (%d) :: %s\n",
               rc, strerrorname_np(errno), errno, strerror(errno) );

      // I'll allow continuation. Process closure should take care of any open
      // file descriptors...
   }

   // Read /proc/[PID]/environ, print out contents, then print environ var and compare.


   return 0;
}
