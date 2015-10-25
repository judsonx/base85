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

  // Should be divisible by 5.
  static const size_t INPUT_BUFFER_DECODE_MAX = 1000;

  char *buffer = NULL;
  char input[INPUT_BUFFER_MAX];

  if (!strcmp (argv[1], "-e"))
  {
    size_t input_cb;
    while ((input_cb = read (0, input, INPUT_BUFFER_MAX)))
    {
      const size_t bufsize = base85_required_buffer_size (input_cb);
      buffer = realloc (buffer, bufsize);
      if (!buffer)
        return 1;

      base85_encode (input, input_cb, buffer);
      printf ("%s", buffer);
    }
  }
  else if (!strcmp (argv[1], "-d"))
  {
    size_t input_cb;
    while ((input_cb = read (0, input, INPUT_BUFFER_DECODE_MAX)))
    {
      size_t cb = 0;
      if (base85_decode (input, input_cb, NULL, &cb))
      {
        fprintf (stderr, "Decoding error\n");
        return 0;
      }
      buffer = realloc (buffer, cb + 4);
      if (!buffer)
        return 1;

      if (base85_decode (input, input_cb, buffer, &cb))
      {
        fprintf (stderr, "Decoding error\n");
        free (buffer);
        return 1;
      }

      (void) write (1, buffer, cb);
    }
  }

  free (buffer);
  return 0;
}
