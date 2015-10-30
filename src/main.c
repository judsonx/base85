/* Copyright 2015 Judson Weissert; See LICENSE file. */

#include "base85.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const size_t ENCODED_LINE_LENGTH = 80;
static const size_t INPUT_BUFFER_MAX = 1024;

/// Wrapper for performing a write operation and returning 1 on error.
#define TRY_WRITE(buf, cb, fh, error_val) do { \
  if (cb != fwrite (buf, 1, cb, fh)) \
  { \
    perror ("* Write error"); \
    return error_val; \
  } \
} while (0);

static int
usage (const char *name)
{
  fprintf (stderr, "Usage: %s -e | -d [input_file [output_file]]\n", name);
  return 2;
}

static int
print_max_width (
  FILE *fh, const char *buf, size_t buf_cb, size_t width, size_t *offset )
{
  // How many bytes were written to the last row.
  size_t rv = (buf_cb + *offset) % width;

  while (buf_cb)
  {
    size_t cb = width - *offset;
    if (buf_cb < cb)
      cb = buf_cb;

    TRY_WRITE (buf, cb, fh, 1)
    if (cb == width - *offset)
      TRY_WRITE ("\n", 1, fh, 1)

    buf_cb -= cb;
    buf += cb;
    *offset = 0;
  }
  *offset = rv;
  return 0;
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
    if (print_max_width (fh_out, out, cb, ENCODED_LINE_LENGTH, &print_offset))
      return B85_E_UNSPECIFIED;
    B85_CLEAR_OUTPUT (ctx);
  }
  rv = B85_ENCODE_LAST (ctx);
  if (rv)
    return rv;

  out = B85_GET_OUTPUT (ctx, &cb);
  if (print_max_width (fh_out, out, cb, ENCODED_LINE_LENGTH, &print_offset))
    return B85_E_UNSPECIFIED;
  if (print_offset)
    TRY_WRITE ("\n", 1, fh_out, B85_E_UNSPECIFIED)

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
    TRY_WRITE (out, out_cb, fh_out, B85_E_UNSPECIFIED)
    B85_CLEAR_OUTPUT (ctx);
  }
  rv = B85_DECODE_LAST (ctx);
  if (rv)
    return rv;

  size_t out_cb;
  out = B85_GET_OUTPUT (ctx, &out_cb);
  TRY_WRITE (out, out_cb, fh_out, B85_E_UNSPECIFIED)

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
  if (B85_E_OK == rv && ferror (fh_in))
    rv = B85_E_UNSPECIFIED;
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

static int
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

static void
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
  if (argc < 2 || argc > 4)
    return usage (argv[0]);

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
