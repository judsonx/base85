#include "base85/base85.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define dimof(x) (sizeof(x) / sizeof(*x))

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

/// Ascii85 decode array. Zero indicates an invalid entry.
/// @see base85_decode_init()
static unsigned char g_ascii85_decode[256];

/// Initializer for g_ascii85_decode (may be called multiple times).
static void
base85_decode_init ()
{
  if (g_ascii85_decode['u'])
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
static int
base85_context_grow (struct base85_context_t *ctx)
{
  // TODO: Refine size, and fallback strategy.
  size_t size = ctx->out_cb * 2;
  ptrdiff_t offset = ctx->out_pos - ctx->out;
  char *buffer = realloc (ctx->out, size);
  if (!buffer)
    return -1;

  ctx->out = buffer;
  ctx->out_cb = size;
  ctx->out_pos = ctx->out + offset;
  return 0;
}

int
base85_context_init (struct base85_context_t *ctx)
{
  static size_t INITIAL_BUFFER_SIZE = 1024;

  base85_decode_init ();

  if (!ctx)
    return -1;

  ctx->pos = 0;
  ctx->out = malloc (INITIAL_BUFFER_SIZE);
  if (!ctx->out)
    return -1;

  ctx->out_pos = ctx->out;
  ctx->out_cb = INITIAL_BUFFER_SIZE;
  return 0;
}

void
base85_context_destroy (struct base85_context_t *ctx)
{
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

static int
base85_encode_strict (struct base85_context_t *ctx)
{
  unsigned int v = 0;
  v |= (unsigned char) ctx->hold[0] << 24;
  v |= (unsigned char) ctx->hold[1] << 16;
  v |= (unsigned char) ctx->hold[2] << 8;
  v |= (unsigned char) ctx->hold[3];

  ctx->pos = 0;

  if (!v)
  {
    if (base85_context_bytes_remaining (ctx) < 1)
    {
      if (base85_context_grow (ctx))
        return -1;
    }
    *ctx->out_pos = 'z';
    ctx->out_pos++;
    return 0;
  }

  if (base85_context_bytes_remaining (ctx) < 5)
  {
    if (base85_context_grow (ctx))
      return -1;
  }

  for (int c = 4; c >= 0; --c)
  {
    ctx->out_pos[c] = g_ascii85_encode[v % 85];
    v /= 85;
  }
  ctx->out_pos += 5;
  return 0;
}

int
base85_encode (const char *b, size_t cb_b, struct base85_context_t *ctx)
{
  while (cb_b--)
  {
    ctx->hold[ctx->pos++] = *b++;
    if (4 == ctx->pos)
    {
      if (base85_encode_strict (ctx))
        return -1;
    }
  }

  return 0;
}

int
base85_encode_last (struct base85_context_t *ctx)
{
  size_t pos = ctx->pos;
  if (!pos)
  {
    if (base85_context_bytes_remaining (ctx) < 1)
    {
      if (base85_context_grow (ctx))
        return -1;
    }
    *ctx->out_pos = 0;
    return 0;
  }

  for (size_t i = pos; i < 4; ++i)
    ctx->hold[i] = 0;

  if (base85_encode_strict (ctx))
    return -1;

  ctx->out_pos -= 4 - pos;
  *ctx->out_pos = 0;
  return 0;
}

/// Decodes exactly 5 bytes from the decode context.
/// @return -1 on overflow.
static int
base85_decode_strict (struct base85_context_t *ctx)
{
  unsigned int v = 0;
  unsigned char x;
  char *b = ctx->hold;

  if (base85_context_bytes_remaining (ctx) < 4)
  {
    if (base85_context_grow (ctx))
      return -1;
  }

  for (int c = 0; c < 4; ++c)
  {
    x = g_ascii85_decode[(unsigned) *b++];
    if (!x--)
      return -1;
    v = v * 85 + x;
  }

  x = g_ascii85_decode[(unsigned) *b];
  if (!x--)
    return -1;

  // Check for overflow.
  if ((0xffffffff / 85 < v) || (0xffffffff - x < (v *= 85)))
    return -1;

  v += x;

  for (int c = 24; c >= 0; c -= 8)
  {
    *(ctx->out_pos) = (v >> c) & 0xff;
    ctx->out_pos++;
  }

  ctx->pos = 0;
  return 0;
}

int
base85_decode (const char *b, size_t cb_b, struct base85_context_t *ctx)
{
  if (!b || !ctx)
    return -1;

  while (cb_b--)
  {
    char c = *b++;

    // Special case for 'z'.
    if ('z' == c && !ctx->pos)
    {
      if (base85_context_bytes_remaining (ctx) < 4)
      {
        if (base85_context_grow (ctx))
          return -1;
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
      if (base85_decode_strict (ctx))
        return -1;
    }
  }

  return 0;
}

int
base85_decode_last (struct base85_context_t *ctx)
{
  size_t pos = ctx->pos;

  if (!pos)
    return 0;

  for (int i = pos; i < 5; ++i)
    ctx->hold[i] = 'u';

  if (base85_decode_strict (ctx))
    return -1;

  ctx->out_pos -= 5 - pos;
  return 0;
}
