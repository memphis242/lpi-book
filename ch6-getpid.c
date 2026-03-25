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

#ifndef _GNU_SOURCE
#define strerrorname_np(str) "strerrorname_np() NOT IMPLEMENTED"
#endif

int main(void)
{
   int rc = 0;

#ifndef NDEBUG

   // Let's read /proc/sys/kernel/pid_max and further assert the assumption
   // that on this system, unsigned long int is big enough to hold any PID.
   (void)printf("Reading /proc/sys/kernel/pid_max to check pid_max limit...\n");

   FILE *fp = fopen("/proc/sys/kernel/pid_max", "r");
   if ( fp == nullptr )
   {
      (void)fprintf(stderr,
               "Error: fopen(\"/proc/sys/kernel/pid_max\", \"r\") failed\n"
               "Returned nullptr :: errno: %s (%d) :: %s\n",
               strerrorname_np(errno), errno, strerror(errno) );

      return 1;
   }

   // 18446744073709551615 is the 64-bit max unsigned long int, which is 20 characters,
   // so, 21 char buffer should be enough. If pid_max was (incredibly) larger than
   // this, we'll know we have something crazy on our hands...
   char buf[21] = {0};
   char * ptr_rc = fgets(buf, sizeof buf, fp);
   if ( ptr_rc == nullptr )
   {
      (void)fprintf(stderr,
               "Error: fgets() failed\n"
               "Returned nullptr (either EOF reached /w no chars read or I/O error occurred)\n" );

      fclose(fp);
      return 2;
   }

   size_t fstr_len = strlen(buf);
   if ( fstr_len < 3 )
   {
      (void)fprintf(stderr,
               "Error: Length of string read from file is less than 3 characters: %zu\n"
               "That's definitely invalid.\n",
               fstr_len );

      fclose(fp);
      return 3;
   }

   long long int pidmax = strtoll(buf, &ptr_rc, 10);
   assert(ptr_rc != nullptr);

   if ( *ptr_rc != '\0' && *ptr_rc != '\n' )
   {
      (void)fprintf(stderr,
               "Error: Invalid character encountered: %c (0x%02X) at position %td\n"
               "String: %s\n",
               *ptr_rc, (int)*ptr_rc,
               ptr_rc - buf,
               buf);

      fclose(fp);
      return 4;
   }
   else if ( pidmax < 300 )
   {
      (void)fprintf(stderr,
               "Error: Read of file returned an invalid limit: %lld\n",
               pidmax );

      fclose(fp);
      return 5;
   }
   else if ( pidmax == LLONG_MAX )
   {
      (void)fprintf(stderr,
               "Error: Read of file returned a limit greater than LLONG_MAX...\n");

      fclose(fp);
      return 6;
   }

   // Now, assert the whole point of this file reading shenanigans...
   assert(pidmax >  300);
   assert((unsigned long)pidmax <= ULONG_MAX);

   (void)puts("");
   (void)printf("%12s: %lu\n", "pid_max", (unsigned long int)pidmax);
   (void)printf("%12s: %lu\n\n", "ULONG_MAX", ULONG_MAX);


   rc = fclose(fp);
   assert(rc == 0); // if fclose() fails, I've set up fp badly.

#endif

   pid_t my_pid = getpid();
   pid_t parent_pid = getppid();

   assert(my_pid > 1);
   assert(parent_pid > 1);
   assert((unsigned long)my_pid <= ULONG_MAX);
   assert((unsigned long)parent_pid <= ULONG_MAX);


   (void)printf("%12s : %lu\n", "PID",        (unsigned long int)my_pid);
   (void)printf("%12s : %lu\n", "Parent PID", (unsigned long int)parent_pid);

   sleep(10);

   return 0;
}
