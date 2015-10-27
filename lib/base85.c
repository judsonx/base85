#include "base85/base85.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define dimof(x) (sizeof(x) / sizeof(*x))

const char *
base85_error_string (b85_result_t val)
{
  static const char *m[] = {
    "Success", // B85_E_OK
    "Out of memory", //B85_E_OOM
    "Byte sequence resulted in an overflow", //B85_E_OVERFLOW
    "Invalid character", // B85_E_INVALID_CHAR
    "Logic error", // B85_E_LOGIC_ERROR
    "API misuse", // B85_E_API_MISUSE
  };

  if (val > 0 && val < dimof (m))
    return m[val];

  return "Unspecified error";
}
/// Ascii85 alphabet.
static const unsigned char g_ascii85_encode[] = {
  '!', '"', '#', '$', '%', '&', '\'', '(',
  ')', '*', '+', ',', '-', '.', '/', '0',
  '1', '2', '3', '4', '5', '6', '7', '8',
  '9', ':', ';', '<', '=', '>', '?', '@',
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', '[', '\\', ']', '^', '_', '`',
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
  'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
  'q', 'r', 's', 't', 'u', 
};

static const char BASE85_ZERO_CHAR = 'z';

/// Ascii85 decode array. Zero indicates an invalid entry.
/// @see base85_decode_init()
static unsigned char g_ascii85_decode[256];

/// Initializer for g_ascii85_decode (may be called multiple times).
static void
base85_decode_init ()
{
  if (g_ascii85_decode[g_ascii85_encode[0]])
    return;

  // NOTE: Assumes g_ascii85_decode[] was implicitly initialized with zeros.
  for (size_t i = 0; i < dimof (g_ascii85_encode); ++i)
  {
    unsigned char c = g_ascii85_encode[i];
    g_ascii85_decode[c] = i + 1;
  }
}

/// Returns true if @a c is a white space character.
static bool
base85_whitespace (char c)
{
  switch (c)
  {
  case ' ':
  case '\n':
  case '\r':
  case '\t':
    return true;
  }
  return false;
}

/// Returns the number of free bytes in the context's output buffer.
static ptrdiff_t
base85_context_bytes_remaining (struct base85_context_t *ctx)
{
  return (ctx->out + ctx->out_cb) - ctx->out_pos;
}

/// Increases the size of the context's output buffer.
static b85_result_t
base85_context_grow (struct base85_context_t *ctx)
{
  // How much additional memory to request if an allocation fails.
  static const size_t SMALL_DELTA = 256;

  // TODO: Refine size, and fallback strategy.
  size_t size = ctx->out_cb * 2;
  ptrdiff_t offset = ctx->out_pos - ctx->out;
  char *buffer = realloc (ctx->out, size);
  if (!buffer)
  {
    // Try a smaller allocation.
    buffer = realloc (ctx->out, ctx->out_cb + SMALL_DELTA);
    if (!buffer)
      return B85_E_OOM;
  }

  ctx->out = buffer;
  ctx->out_cb = size;
  ctx->out_pos = ctx->out + offset;
  return B85_E_OK;
}

char *
base85_get_output (struct base85_context_t *ctx, size_t *cb)
{
  if (!ctx || !cb)
    return NULL;

  *cb = ctx->out_pos - ctx->out;
  return ctx->out;
}

void
base85_clear_output (struct base85_context_t *ctx)
{
  if (!ctx)
    return;

  ctx->out_pos = ctx->out;
}

b85_result_t
base85_context_init (struct base85_context_t *ctx)
{
  static size_t INITIAL_BUFFER_SIZE = 1024;

  base85_decode_init ();

  if (!ctx)
    return B85_E_API_MISUSE;

  ctx->out_cb = 0;
  ctx->out_pos = NULL;
  ctx->processed = 0;
  ctx->pos = 0;

  ctx->out = malloc (INITIAL_BUFFER_SIZE);
  if (!ctx->out)
    return B85_E_OOM;

  ctx->out_pos = ctx->out;
  ctx->out_cb = INITIAL_BUFFER_SIZE;
  return B85_E_OK;
}

void
base85_context_destroy (struct base85_context_t *ctx)
{
  if (!ctx)
    return;

  char *out = ctx->out;
  ctx->out = NULL;
  ctx->out_pos = NULL;
  ctx->out_cb = 0;
  free (out);
}

size_t
base85_required_buffer_size (size_t input_size)
{
  size_t s = ((input_size + 3) / 4) * 4;
  return 1 + s + (s / 4);
}

static b85_result_t
base85_encode_strict (struct base85_context_t *ctx)
{
  unsigned char *h = ctx->hold;
  unsigned int v = h[0] << 24 | h[1] << 16 | h[2] << 8 | h[3];

  ctx->pos = 0;

  if (!v)
  {
    if (base85_context_bytes_remaining (ctx) < 1)
    {
      b85_result_t rv = base85_context_grow (ctx);
      if (rv) return rv;
    }
    *ctx->out_pos = BASE85_ZERO_CHAR;
    ctx->out_pos++;
    return B85_E_OK;
  }

  if (base85_context_bytes_remaining (ctx) < 5)
  {
    b85_result_t rv = base85_context_grow (ctx);
    if (rv) return rv;
  }

  for (int c = 4; c >= 0; --c)
  {
    ctx->out_pos[c] = g_ascii85_encode[v % 85];
    v /= 85;
  }
  ctx->out_pos += 5;
  return B85_E_OK;
}

b85_result_t
base85_encode (const char *b, size_t cb_b, struct base85_context_t *ctx)
{
  if (!ctx || (cb_b && !b))
    return B85_E_API_MISUSE;

  if (!cb_b)
    return B85_E_OK;

  while (cb_b--)
  {
    ctx->hold[ctx->pos++] = *b++;
    ctx->processed++;
    if (4 == ctx->pos)
    {
      b85_result_t rv = base85_encode_strict (ctx);
      if (rv) return rv;
    }
  }

  return B85_E_OK;
}

b85_result_t
base85_encode_last (struct base85_context_t *ctx)
{
  if (!ctx)
    return B85_E_API_MISUSE;

  b85_result_t rv = B85_E_UNSPECIFIED;
  size_t pos = ctx->pos;
  if (!pos)
  {
    if (base85_context_bytes_remaining (ctx) < 1)
    {
      rv = base85_context_grow (ctx);
      if (rv) return rv;
    }
    *ctx->out_pos = 0;
    return B85_E_OK;
  }

  for (size_t i = pos; i < 4; ++i)
    ctx->hold[i] = 0;

  rv = base85_encode_strict (ctx);
  if (rv) return rv;

  ctx->out_pos -= 4 - pos;
  *ctx->out_pos = 0;
  return B85_E_OK;
}

/// Decodes exactly 5 bytes from the decode context.
static b85_result_t
base85_decode_strict (struct base85_context_t *ctx)
{
  unsigned int v = 0;
  unsigned char x;
  unsigned char *b = ctx->hold;

  if (base85_context_bytes_remaining (ctx) < 4)
  {
    b85_result_t rv = base85_context_grow (ctx);
    if (rv) return rv;
  }

  for (int c = 0; c < 4; ++c)
  {
    x = g_ascii85_decode[(unsigned) *b++];
    if (!x--)
      return B85_E_INVALID_CHAR;
    v = v * 85 + x;
  }

  x = g_ascii85_decode[(unsigned) *b];
  if (!x--)
    return B85_E_INVALID_CHAR;

  // Check for overflow.
  if ((0xffffffff / 85 < v) || (0xffffffff - x < (v *= 85)))
    return B85_E_OVERFLOW;

  v += x;

  for (int c = 24; c >= 0; c -= 8)
  {
    *(ctx->out_pos) = (v >> c) & 0xff;
    ctx->out_pos++;
  }

  ctx->pos = 0;
  return B85_E_OK;
}

b85_result_t
base85_decode (const char *b, size_t cb_b, struct base85_context_t *ctx)
{
  if (!ctx || (cb_b && !b))
    return B85_E_API_MISUSE;

  if (!cb_b)
    return B85_E_OK;

  while (cb_b--)
  {
    char c = *b++;
    ctx->processed++;

    // Special case for 'z'.
    if (BASE85_ZERO_CHAR == c && !ctx->pos)
    {
      if (base85_context_bytes_remaining (ctx) < 4)
      {
        b85_result_t rv = base85_context_grow (ctx);
        if (rv) return rv;
      }

      memset (ctx->out_pos, 0, 4);
      ctx->out_pos += 4;
      continue;
    }

    if (base85_whitespace (c))
      continue;

    ctx->hold[ctx->pos++] = c;
    if (5 == ctx->pos)
    {
      b85_result_t rv = base85_decode_strict (ctx);
      if (rv) return rv;
    }
  }

  return B85_E_OK;
}

b85_result_t
base85_decode_last (struct base85_context_t *ctx)
{
  if (!ctx)
    return B85_E_API_MISUSE;

  size_t pos = ctx->pos;

  if (!pos)
    return B85_E_OK;

  for (int i = pos; i < 5; ++i)
    ctx->hold[i] = g_ascii85_encode[dimof (g_ascii85_encode) - 1];

  b85_result_t rv = base85_decode_strict (ctx);
  if (rv) return rv;

  ctx->out_pos -= 5 - pos;
  return B85_E_OK;
}
