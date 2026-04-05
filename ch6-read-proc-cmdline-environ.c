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

constexpr size_t MAX_PID_LEN      = 7 + 1; // Based on /proc/sys/kernel/pid_max + 1 for nul
constexpr size_t MAX_ENV_VAR_LEN  = 2048;  // I checked this via `awk '{ print length " | " $0 }' <(env) | sort -n -s`
constexpr size_t MAX_NUM_ENV_VARS = 128;   // Checked via `env | wc -l`

extern char ** environ;

int main(int argc, char * argv[])
{
   int rc = 0;

   pid_t my_pid = getpid();

   assert(my_pid > 1);
   assert((unsigned long)my_pid <= LONG_MAX);

   // Read /proc/[PID]/cmdline, print out contents, then print argv and compare.

   // First, open the file
   char cmdline_fname[ sizeof("/proc/") + MAX_PID_LEN + sizeof("/cmdline") - 2 + 1 ] = {0};
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
               "ERROR: fopen(\"%s\", \"r\") failed\n"
               "Returned nullptr :: errno: %s (%d) :: %s\n",
               cmdline_fname,
               strerrorname_np(errno), errno, strerror(errno) );

      return 1;
   }

   // Then read file contents
   char cmdline_fbuf[1024] = {0}; // I'm not expecting more than that for this test program
   nbytes = 0;
   int c;
   while ( (c = fgetc(fp)) != EOF && nbytes < (int)(sizeof cmdline_fbuf) )
      cmdline_fbuf[nbytes++] = (char)c;

   if ( nbytes >= (int)(sizeof cmdline_fbuf) )
   {
      (void)fprintf(stderr,
               "ERROR: cmdline file too large.\n"
               "       Program only written to support up to %zu bytes in cmdline file.",
               sizeof cmdline_fbuf );

      (void)fclose(fp);
      return 2;
   }

   // Then split out cmdline arguments into argv-like array
   assert(nbytes <= (int)(sizeof cmdline_fbuf));

   char cmdline_args[8][128] = {0};
   static_assert(sizeof cmdline_args == sizeof cmdline_fbuf);

   char prev_char = '\0';
   size_t nargs=0, arg_int_idx=0;
   for ( size_t fbuf_idx=0;
         fbuf_idx < (size_t)nbytes && nargs < ARR_LEN(cmdline_args) && arg_int_idx < ARR_LEN(cmdline_args[0]);
         ++fbuf_idx )
   {
      char c = cmdline_fbuf[fbuf_idx];
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
   // No args we parsed should be an empty string
   for ( size_t i=0; i < nargs; ++i )
      assert( strnlen(cmdline_args[i], ARR_LEN(cmdline_args[0])) > 0 );
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
               "ERROR: Unable to fclose() cmdline file.\n"
               "       fclose() returned: %d :: errno %s (%d) :: %s\n",
               rc, strerrorname_np(errno), errno, strerror(errno) );

      // I'll allow continuation. Process closure should take care of any open
      // file descriptors...
   }

   (void)puts("");
   (void)puts("----------------------------------------------------------------------");
   (void)puts("");

   // Read /proc/[PID]/environ, print out contents, then print environ var and compare.

   // First, open the file
   char environ_fname[ sizeof("/proc/") + MAX_PID_LEN + sizeof("/environ") - 2 + 1 ] = {0};
   nbytes = snprintf(environ_fname, sizeof environ_fname,
                     "/proc/%d/environ",
                     my_pid);

   static_assert(sizeof environ_fname < INT_MAX);
   assert(nbytes < (int)(sizeof environ_fname));

   (void)printf("Reading and parsing %s...\n\n", environ_fname);

   fp = fopen(environ_fname, "r");
   if ( fp == nullptr )
   {
      (void)fprintf(stderr,
               "ERROR: fopen(\"%s\", \"r\") failed\n"
               "Returned nullptr :: errno: %s (%d) :: %s\n",
               environ_fname,
               strerrorname_np(errno), errno, strerror(errno) );

      return 1;
   }

   // Then read file contents
   char environ_fbuf[MAX_NUM_ENV_VARS * MAX_ENV_VAR_LEN ] = {0};
   static_assert( sizeof(environ_fbuf) <= INT_MAX );

   // Let's try fread()+feof()+ferror() here instead of fgetc()...
   nbytes = (int)fread( environ_fbuf,
                        1, sizeof environ_fbuf,
                        fp );
   if ( nbytes == 0 )
   {
      (void)printf("INFO: fread() of %s returned 0 bytes.\n"
                   "      Most likely, program was invoked /w env -i\n\n",
                   environ_fname);
   }
   else if ( !feof(fp) || nbytes >= (int)sizeof(environ_fbuf) )
   {
      (void)fprintf(stderr,
               "ERROR: Buffer was not large enough to fit file content.\n"
               "       Buffer Size: %zu\n\n",
               sizeof environ_fbuf);

      (void)fclose(fp);
      return 2;
   }

   // Then split out environ arguments into argv-like array
   assert(nbytes <= (int)(sizeof environ_fbuf));

   char env_vars_from_file[MAX_NUM_ENV_VARS][MAX_ENV_VAR_LEN] = {0};
   static_assert(sizeof env_vars_from_file == sizeof environ_fbuf);

   prev_char = '\0';
   size_t nvars=0, envvar_str_idx=0;
   for ( size_t fbuf_idx=0;
         fbuf_idx < (size_t)nbytes && nvars < ARR_LEN(env_vars_from_file) && envvar_str_idx < ARR_LEN(env_vars_from_file[0]);
         ++fbuf_idx )
   {
      char c = environ_fbuf[fbuf_idx];
      env_vars_from_file[nvars][envvar_str_idx] = c;

      if ( c == '\0' )
      {
         ++nvars;
         envvar_str_idx = 0;

         // Shouldn't have two nul characters in a row or as the first character...
         assert(prev_char != '\0');
      }
      else
      {
         ++envvar_str_idx;
      }

      prev_char = c;
   }

   assert(nvars <= ARR_LEN(env_vars_from_file));
   assert(envvar_str_idx <= ARR_LEN(env_vars_from_file[0]));
#ifndef NDEBUG
   // No vars we parsed should be an empty string
   for ( size_t i=0; i < nvars; ++i )
      assert( strnlen(env_vars_from_file[i], ARR_LEN(env_vars_from_file[0])) > 0 );
#endif

   // Now print out what we read, alongside environ, and also compare simultaneously.
   char ** environ_var = environ;
   for ( size_t i=0; i < nvars; ++i, environ_var++ )
   {
      // If I wrote my parsing of the environ file correctly, I should not run
      // into an issue where environ_var was incremented past the end of environ.
      assert(environ_var != nullptr);

      (void)printf("env_vars_from_file[%zu]: %s\n"
                   "           environ[%zu]: %s\n",
                   i, env_vars_from_file[i],
                   i, *environ_var );

      bool args_equivalent = strcmp((char *)env_vars_from_file[i], *environ_var) == 0;
      if ( !args_equivalent )
         puts("Mismatch!");

      (void)puts("");

      (void)fflush(stdout);
   }

   // Now close environ file.
   rc = fclose(fp);
   if ( rc != 0 )
   {
      (void)fprintf(stderr,
               "ERROR: Unable to fclose() environ file.\n"
               "       fclose() returned: %d :: errno %s (%d) :: %s\n",
               rc, strerrorname_np(errno), errno, strerror(errno) );

      // I'll allow continuation. Process closure should take care of any open
      // file descriptors...
   }

   return 0;
}
