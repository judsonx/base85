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
  fprintf (stderr, "Usage: %s -e | -d [input_file [output_file]]\n", name);
  return 2;
}

static size_t
print_max_width (
  FILE *fh, const char *buf, size_t buf_cb, size_t width, size_t offset )
{
  // How many bytes were written to the last row.
  size_t rv = (buf_cb + offset) % width;

  while (buf_cb)
  {
    size_t cb = width - offset;
    if (buf_cb < cb)
      cb = buf_cb;

    (void) fwrite (buf, 1, cb, fh);
    if (cb == width - offset)
      (void) fwrite ("\n", 1, 1, fh);

    buf_cb -= cb;
    buf += cb;
    offset = 0;
  }
  return rv;
}

static b85_result_t
b85_encode (struct base85_context_t *ctx, FILE *fh_in, FILE *fh_out)
{
  char input[INPUT_BUFFER_MAX];
  b85_result_t rv = B85_E_UNSPECIFIED;

  size_t print_offset = 0;
  size_t cb = 0;
  size_t input_cb;
  char *out = NULL;
  while ((input_cb = fread (input, 1, INPUT_BUFFER_MAX, fh_in)))
  {
    rv = B85_ENCODE (input, input_cb, ctx);
    if (rv)
      return rv;

    out = B85_GET_OUTPUT (ctx, &cb);
    print_offset = print_max_width (
      fh_out, out, cb, ENCODED_LINE_LENGTH, print_offset
    );
    B85_CLEAR_OUTPUT (ctx);
  }
  rv = B85_ENCODE_LAST (ctx);
  if (rv)
    return rv;

  out = B85_GET_OUTPUT (ctx, &cb);
  (void) print_max_width (fh_out, out, cb, ENCODED_LINE_LENGTH, print_offset);
  (void) fwrite ("\n", 1, 1, fh_out);

  return B85_E_OK;
}

static b85_result_t
b85_decode (struct base85_context_t *ctx, FILE *fh_in, FILE *fh_out)
{
  char input[INPUT_BUFFER_MAX];
  b85_result_t rv = B85_E_UNSPECIFIED;

  size_t input_cb;
  char *out = NULL;
  while ((input_cb = fread (input, 1, INPUT_BUFFER_MAX, fh_in)))
  {
    rv = B85_DECODE (input, input_cb, ctx);
    if (rv)
      return rv;

    size_t out_cb;
    out = B85_GET_OUTPUT (ctx, &out_cb);
    (void) fwrite (out, 1, out_cb, fh_out);
    B85_CLEAR_OUTPUT (ctx);
  }
  rv = B85_DECODE_LAST (ctx);
  if (rv)
    return rv;

  size_t out_cb;
  out = B85_GET_OUTPUT (ctx, &out_cb);
  (void) fwrite (out, 1, out_cb, fh_out);

  return B85_E_OK;
}

typedef b85_result_t (*handler_t) (struct base85_context_t *, FILE *, FILE *);

static b85_result_t
b85_wrapper (handler_t handler, FILE *fh_in, FILE *fh_out)
{
  struct base85_context_t ctx;
  b85_result_t rv = B85_CONTEXT_INIT (&ctx);
  if (B85_E_OK == rv)
    rv = handler (&ctx, fh_in, fh_out);
  if (rv)
  {
    fprintf (
      stderr, "* Error[%d]: %s. [position: %zu]\n", rv,
      B85_ERROR_STRING (rv),
      B85_GET_PROCESSED (&ctx)
    );
  }
  B85_CONTEXT_DESTROY (&ctx);
  return rv;
}

int
open_file_handles (int argc, char *argv[], FILE **fh_in, FILE **fh_out)
{
  FILE *in = NULL;
  FILE *out = NULL;

  switch (argc)
  {
  case 4:
    out = fopen (argv[3], "wb");
    if (!out)
    {
      perror ("* Output fopen() error");
      return 1;
    }
  case 3:
    in = fopen (argv[2], "rb");
    if (!in)
    {
      perror ("* Input fopen() error");
      (void) fclose (out);
      return 1;
    }
  case 2:
    break;
  default:
    return usage (argv[0]);
  }

  if (in)
    *fh_in = in;
  if (out)
    *fh_out = out;
  return 0;
}

void
close_file_handles (int argc, FILE **fh_in, FILE **fh_out)
{
  switch (argc)
  {
  case 4:
    (void) fclose (*fh_out);
  case 3:
    (void) fclose (*fh_in);
  }

  *fh_in = NULL;
  *fh_out = NULL;
}

int
main (int argc, char *argv[])
{
  FILE *fh_in = stdin;
  FILE *fh_out = stdout;

  b85_result_t rv = B85_E_UNSPECIFIED;
  if (!strcmp (argv[1], "-e"))
  {
    int result = open_file_handles (argc, argv, &fh_in, &fh_out);
    if (result)
      return result;
    rv = b85_wrapper (b85_encode, fh_in, fh_out);
  }
  else if (!strcmp (argv[1], "-d"))
  {
    int result = open_file_handles (argc, argv, &fh_in, &fh_out);
    if (result)
      return result;
    rv = b85_wrapper (b85_decode, fh_in, fh_out);
  }
  else
    return usage (argv[0]);

  close_file_handles (argc, &fh_in, &fh_out);
  return rv;
}
