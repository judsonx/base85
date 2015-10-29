#include "base85/base85.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const size_t ENCODED_LINE_LENGTH = 80;
static const size_t INPUT_BUFFER_MAX = 1024;

static int
usage (const char *name)
{
  fprintf (stderr, "Usage: %s -e | -d\n", name);
  return 2;
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

static b85_result_t
b85_encode (struct base85_context_t *ctx)
{
  char input[INPUT_BUFFER_MAX];
  b85_result_t rv = B85_E_UNSPECIFIED;

  size_t print_offset = 0;
  size_t cb = 0;
  size_t input_cb;
  char *out = NULL;
  while ((input_cb = read (0, input, INPUT_BUFFER_MAX)))
  {
    rv = B85_ENCODE (input, input_cb, ctx);
    if (rv)
      return rv;

    out = B85_GET_OUTPUT (ctx, &cb);
    print_offset = print_max_width (
      out, cb, ENCODED_LINE_LENGTH, print_offset
    );
    B85_CLEAR_OUTPUT (ctx);
  }
  rv = B85_ENCODE_LAST (ctx);
  if (rv)
    return rv;

  out = B85_GET_OUTPUT (ctx, &cb);
  (void) print_max_width (out, cb, ENCODED_LINE_LENGTH, print_offset);
  (void) putchar ('\n');

  return B85_E_OK;
}

static b85_result_t
b85_decode (struct base85_context_t *ctx)
{
  char input[INPUT_BUFFER_MAX];
  b85_result_t rv = B85_E_UNSPECIFIED;

  size_t input_cb;
  char *out = NULL;
  while ((input_cb = read (0, input, INPUT_BUFFER_MAX)))
  {
    rv = B85_DECODE (input, input_cb, ctx);
    if (rv)
      return rv;

    size_t out_cb;
    out = B85_GET_OUTPUT (ctx, &out_cb);
    (void) write (1, out, out_cb);
    B85_CLEAR_OUTPUT (ctx);
  }
  rv = B85_DECODE_LAST (ctx);
  if (rv)
    return rv;

  size_t out_cb;
  out = B85_GET_OUTPUT (ctx, &out_cb);
  (void) write (1, out, out_cb);

  return B85_E_OK;
}

typedef b85_result_t (*handler_t) (struct base85_context_t *);

static b85_result_t
b85_wrapper (handler_t handler)
{
  struct base85_context_t ctx;
  b85_result_t rv = B85_CONTEXT_INIT (&ctx);
  if (B85_E_OK == rv)
    rv = handler (&ctx);
  if (rv)
  {
    fprintf (
      stderr, "* Error[%d]: %s. [position: %zu]\n", rv,
      B85_ERROR_STRING (rv),
      B85_GET_PROCESSED (&ctx)
    );
  }
  B85_CONTEXT_DESTROY (&ctx);
  return (B85_E_OK == rv) ? 0 : 1;
}

int
main (int argc, char *argv[])
{
  if (2 != argc)
    return usage (argv[0]);

  if (!strcmp (argv[1], "-e"))
    return b85_wrapper (b85_encode);
  else if (!strcmp (argv[1], "-d"))
    return b85_wrapper (b85_decode);

  return usage (argv[0]);
}
