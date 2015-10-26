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

int
main (int argc, char *argv[])
{
  if (2 != argc)
    return usage (argv[0]);

  // Should be divisible by 4.
  static const size_t INPUT_BUFFER_MAX = 1024;

  char input[INPUT_BUFFER_MAX];

  if (!strcmp (argv[1], "-e"))
  {
    char *buffer = NULL;
    size_t input_cb;
    while ((input_cb = read (0, input, INPUT_BUFFER_MAX)))
    {
      const size_t bufsize = base85_required_buffer_size (input_cb);
      buffer = realloc (buffer, bufsize);
      if (!buffer)
        return 1;

      base85_encode (input, input_cb, buffer);
      printf ("%s\n", buffer);
    }
    free (buffer);
  }
  else if (!strcmp (argv[1], "-d"))
  {
    struct base85_decode_context_t ctx;
    if (base85_decode_context_init (&ctx))
      return 1;

    size_t input_cb;
    while ((input_cb = read (0, input, INPUT_BUFFER_MAX)))
    {
      if (base85_decode (input, input_cb, &ctx))
      {
        fprintf (stderr, "Decoding error\n");
        base85_decode_context_destroy (&ctx);
        return 1;
      }
    }
    if (base85_decode_last (&ctx))
    {
      fprintf (stderr, "Decoding error (last)\n");
      base85_decode_context_destroy (&ctx);
      return 1;
    } 
    size_t out_cb = ctx.out_pos - ctx.out;
    (void) write (1, ctx.out, out_cb);
    base85_decode_context_destroy (&ctx);
  }

  return 0;
}
