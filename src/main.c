#include "base85/base85.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const size_t ENCODED_LINE_LENGTH = 80;

static int
usage (const char *name)
{
  fprintf (stderr, "Usage: %s -e | -d\n", name);
  return 2;
}

static void
report_error (b85_result_t val)
{
  fprintf (stderr, "* Error: %s.\n", base85_error_string (val));
}

static size_t
print_max_width (const char *buf, size_t buf_cb, size_t width, size_t offset)
{
  // How many bytes were written to the last row.
  size_t rv = (buf_cb + offset) % width;

  while (buf_cb)
  {
    size_t cb = width - offset;
    if (buf_cb < cb)
      cb = buf_cb;

    (void) write (1, buf, cb);
    if (cb == width - offset)
      (void) putchar ('\n');

    buf_cb -= cb;
    buf += cb;
    offset = 0;
  }
  return rv;
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
  rv = base85_context_init (&ctx);
  if (rv)
    goto error_exit;

  if (!strcmp (argv[1], "-e"))
  {
    size_t print_offset = 0;
    size_t input_cb;
    while ((input_cb = read (0, input, INPUT_BUFFER_MAX)))
    {
      rv = base85_encode (input, input_cb, &ctx);
      if (rv)
        goto error_exit;

      size_t cb = ctx.out_pos - ctx.out;
      print_offset = print_max_width (
        ctx.out, cb, ENCODED_LINE_LENGTH, print_offset
      );
      ctx.out_pos = ctx.out;
    }
    rv = base85_encode_last (&ctx);
    if (rv)
      goto error_exit;

    size_t cb = ctx.out_pos - ctx.out;
    (void) print_max_width (ctx.out, cb, ENCODED_LINE_LENGTH, print_offset);
    (void) putchar ('\n');
  }
  else if (!strcmp (argv[1], "-d"))
  {
    size_t input_cb;
    while ((input_cb = read (0, input, INPUT_BUFFER_MAX)))
    {
      rv = base85_decode (input, input_cb, &ctx);
      if (rv)
        goto error_exit;

      size_t out_cb = ctx.out_pos - ctx.out;
      (void) write (1, ctx.out, out_cb);
      ctx.out_pos = ctx.out;
    }
    rv = base85_decode_last (&ctx);
    if (rv)
      goto error_exit;

    size_t out_cb = ctx.out_pos - ctx.out;
    (void) write (1, ctx.out, out_cb);
  }

  base85_context_destroy (&ctx);
  return 0;

error_exit:
  report_error (rv);
  base85_context_destroy (&ctx);
  return 1;
}
