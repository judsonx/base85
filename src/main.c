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

static void
report_error (b85_result_t val)
{
  if (B85_E_OK == val)
    return;

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

static b85_result_t
b85_encode (struct base85_context_t *ctx)
{
  char input[INPUT_BUFFER_MAX];
  b85_result_t rv = B85_E_UNSPECIFIED;

  size_t print_offset = 0;
  size_t cb = 0;
  size_t input_cb;
  while ((input_cb = read (0, input, INPUT_BUFFER_MAX)))
  {
    rv = base85_encode (input, input_cb, ctx);
    if (rv)
      return rv;

    cb = ctx->out_pos - ctx->out;
    print_offset = print_max_width (
      ctx->out, cb, ENCODED_LINE_LENGTH, print_offset
    );
    ctx->out_pos = ctx->out;
  }
  rv = base85_encode_last (ctx);
  if (rv)
    return rv;

  cb = ctx->out_pos - ctx->out;
  (void) print_max_width (ctx->out, cb, ENCODED_LINE_LENGTH, print_offset);
  (void) putchar ('\n');

  return B85_E_OK;
}

static b85_result_t
b85_decode (struct base85_context_t *ctx)
{
  char input[INPUT_BUFFER_MAX];
  b85_result_t rv = B85_E_UNSPECIFIED;

  size_t input_cb;
  while ((input_cb = read (0, input, INPUT_BUFFER_MAX)))
  {
    rv = base85_decode (input, input_cb, ctx);
    if (rv)
      return rv;

    size_t out_cb = ctx->out_pos - ctx->out;
    (void) write (1, ctx->out, out_cb);
    ctx->out_pos = ctx->out;
  }
  rv = base85_decode_last (ctx);
  if (rv)
    return rv;

  size_t out_cb = ctx->out_pos - ctx->out;
  (void) write (1, ctx->out, out_cb);

  return B85_E_OK;
}

typedef b85_result_t (*handler_t) (struct base85_context_t *);

static b85_result_t
b85_wrapper (handler_t handler)
{
  struct base85_context_t ctx;
  b85_result_t rv = base85_context_init (&ctx);
  if (B85_E_OK == rv)
    rv = handler (&ctx);
  base85_context_destroy (&ctx);
  return rv;
}

int
main (int argc, char *argv[])
{
  if (2 != argc)
    return usage (argv[0]);

  b85_result_t rv = B85_E_UNSPECIFIED;
  if (!strcmp (argv[1], "-e"))
  {
    rv = b85_wrapper (b85_encode);
    report_error (rv);
  }
  else if (!strcmp (argv[1], "-d"))
  {
    rv = b85_wrapper (b85_decode);
    report_error (rv);
  }
  else
  {
    return usage (argv[0]);
  }

  return (B85_E_OK == rv) ? 0 : 1;
}
