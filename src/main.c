#include "base85/base85.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
usage (const char *name)
{
  fprintf (stderr, "Usage: %s -e | -d\n", name);
  return 2;
}

void
report_error (b85_result_t val)
{
  fprintf (stderr, "* Error: %s.\n", base85_error_string (val));
}

int
main (int argc, char *argv[])
{
  if (2 != argc)
    return usage (argv[0]);

  // Should be divisible by 4.
  static const size_t INPUT_BUFFER_MAX = 1024;

  b85_result_t rv = B85_E_UNSPECIFIED;
  char input[INPUT_BUFFER_MAX];

  struct base85_context_t ctx;
  if (base85_context_init (&ctx))
    return 1;

  if (!strcmp (argv[1], "-e"))
  {
    size_t input_cb;
    while ((input_cb = read (0, input, INPUT_BUFFER_MAX)))
    {
      rv = base85_encode (input, input_cb, &ctx);
      if (rv)
      {
        report_error (rv);
        base85_context_destroy (&ctx);
        return 1;
      }
    }
    rv = base85_encode_last (&ctx);
    if (rv)
    {
      report_error (rv);
      base85_context_destroy (&ctx);
      return 1;
    }

    printf ("%s\n", ctx.out);
  }
  else if (!strcmp (argv[1], "-d"))
  {

    size_t input_cb;
    while ((input_cb = read (0, input, INPUT_BUFFER_MAX)))
    {
      rv = base85_decode (input, input_cb, &ctx);
      if (rv)
      {
        report_error (rv);
        base85_context_destroy (&ctx);
        return 1;
      }
    }
    rv = base85_decode_last (&ctx);
    if (rv)
    {
      report_error (rv);
      base85_context_destroy (&ctx);
      return 1;
    } 
    size_t out_cb = ctx.out_pos - ctx.out;
    (void) write (1, ctx.out, out_cb);
  }

  base85_context_destroy (&ctx);
  return 0;
}
