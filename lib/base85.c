#include "base85/base85.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define dimof(x) (sizeof(x) / sizeof(*x))

/// Ascii85 decode array. Zero indicates an invalid entry.
/// @see base85_decode_init()
static unsigned char g_ascii85_decode[256];

static const char B85_HEADER0 = '<';
static const char B85_HEADER1 = '~';
static const char B85_FOOTER0 = '~';
static const char B85_FOOTER1 = '>';
static const char BASE85_ZERO_CHAR = 'z';

typedef enum
{
  B85_S_START = 0,
  B85_S_NO_HEADER,
  B85_S_HEADER0,
  B85_S_HEADER,
  B85_S_FOOTER0,
  B85_S_FOOTER,
  B85_S_INVALID,
  B85_S_END
} b85_state_t;

/// State transitions for handling ascii85 header/footer.
static bool
base85_handle_state (char c, struct base85_context_t *ctx)
{
  unsigned char *state = &ctx->state;
  switch (*state)
  {
  case B85_S_START:
    if (B85_HEADER0 == c)
    {
      *state = B85_S_HEADER0;
      return true;
    }

    *state = B85_S_NO_HEADER;
    return false;

  case B85_S_NO_HEADER:
    return false;

  case B85_S_HEADER0:
    if (B85_HEADER1 == c)
    {
      *state = B85_S_HEADER;
      return true;
    }

    // Important, have to add B85_HEADER0 char to the hold.
    // NOTE: Assumes that B85_HEADER0 is in the alphabet.
    ctx->hold[ctx->pos++] = g_ascii85_decode[(unsigned) B85_HEADER0] - 1;
    *state = B85_S_NO_HEADER;
    return false;

  case B85_S_HEADER:
    if (B85_FOOTER0 == c)
    {
      *state = B85_S_FOOTER0;
      return true;
    }
    return false;

  case B85_S_FOOTER0:
    if (B85_FOOTER1 == c)
    {
      *state = B85_S_FOOTER;
      return true;
    }
    *state = B85_S_INVALID;
    return true;

  case B85_S_FOOTER:
  case B85_S_INVALID:
    break;
  }

  return true;
}

const char *
base85_debug_error_string (b85_result_t val)
{
  static const char *m[] = {
    "B85_E_OK",
    "B85_E_OOM",
    "B85_E_OVERFLOW",
    "B85_E_INVALID_CHAR",
    "B85_E_BAD_FOOTER",
    "B85_E_LOGIC_ERROR",
    "B85_E_API_MISUSE",
  };

  if (val >= 0 && val < dimof (m))
    return m[val];

  return "B85_E_UNSPECIFIED";
}

const char *
base85_error_string (b85_result_t val)
{
  static const char *m[] = {
    "Success", // B85_E_OK
    "Out of memory", //B85_E_OOM
    "Byte sequence resulted in an overflow", //B85_E_OVERFLOW
    "Invalid character", // B85_E_INVALID_CHAR
    "Missing or invalid footer", // B85_E_BAD_FOOTER
    "Logic error", // B85_E_LOGIC_ERROR
    "API misuse", // B85_E_API_MISUSE
  };

  if (val >= 0 && val < dimof (m))
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

/// True if @a state is "critical", i.e. when whitespace is important.
static inline bool
base85_critical_state (b85_state_t state)
{
  return state == B85_S_HEADER0 || state == B85_S_FOOTER0;
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

static bool
base85_can_skip (char c, b85_state_t state)
{
  return !base85_critical_state (state) && base85_whitespace (c);
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
  ctx->state = B85_S_START;

  ctx->out = malloc (INITIAL_BUFFER_SIZE);
  if (!ctx->out)
    return B85_E_OOM;

  ctx->out_pos = ctx->out;
  ctx->out_cb = INITIAL_BUFFER_SIZE;
  return B85_E_OK;
}

void
base85_context_reset (struct base85_context_t *ctx)
{
  if (!ctx)
    return;

  // Do not reset out or out_cb.

  ctx->out_pos = ctx->out;
  ctx->processed = 0;
  ctx->pos = 0;
  ctx->state = B85_S_START;
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
  uint32_t v = h[0] << 24 | h[1] << 16 | h[2] << 8 | h[3];

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
  uint32_t v = 0;
  unsigned char *b = ctx->hold;

  if (base85_context_bytes_remaining (ctx) < 4)
  {
    b85_result_t rv = base85_context_grow (ctx);
    if (rv) return rv;
  }

  for (int c = 0; c < 4; ++c)
    v = v * 85 + b[c];

  // Check for overflow.
  if ((0xffffffff / 85 < v) || (0xffffffff - b[4] < (v *= 85)))
    return B85_E_OVERFLOW;

  v += b[4];

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
    // Skip all input if a valid footer has already been found.
    if (B85_S_FOOTER == ctx->state)
      return B85_E_OK;

    if (B85_S_INVALID == ctx->state)
      return B85_E_BAD_FOOTER;

    char c = *b++;
    ctx->processed++;

    if (base85_can_skip (c, (b85_state_t) ctx->state))
      continue;

    if (base85_handle_state (c, ctx))
      continue;

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

    unsigned char x = g_ascii85_decode[(unsigned) c];
    if (!x--)
      return B85_E_INVALID_CHAR;

    ctx->hold[ctx->pos++] = x;
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

  if (B85_S_START == ctx->state)
    return B85_E_OK;

  if ((B85_S_FOOTER != ctx->state) && (B85_S_NO_HEADER != ctx->state))
    return B85_E_BAD_FOOTER;

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
