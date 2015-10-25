#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
base85_skipws (const char **b, size_t *cb_b)
{
  while (*cb_b)
  {
    if (!base85_whitespace (**b))
      return true;
    ++(*b);
    --(*cb_b);
  }
  return false;
}

static bool
base85_decode_valid_char (char c)
{
  return (c > 32) && (c < 118);
}

/// Required buffer size to hold the base85 encoded result for an input string
/// with length @a input_size. Note: includes room for NUL.
size_t
base85_required_buffer_size (size_t input_size)
{
  size_t s = ((input_size + 3) / 4) * 4;
  return 1 + s + (s / 4);
}

/// Encodes @a cb_b characters from @a b, and stores the result in @a out.
/// @pre @a b must contain at least @a cb_b bytes, and @a out must be large
/// enough to hold the result (including room for NUL terminator). 
void
base85_encode (const char *b, size_t cb_b, char *out)
{
  unsigned int v;
  while (cb_b)
  {
    v = 0;
    size_t start_cb = cb_b;
    size_t padding = 0;
    for (int c = 24; c >= 0; c -= 8)
    {
      v |= *b++ << c;
      if (!--cb_b)
      {
        padding = 4 - start_cb;
        break;
      }
    }

    if (!v)
    {
      *out++ = 'z';
      continue;
    }

    for (int c = 4; c >= 0; --c)
    {
      out[c] = g_ascii85_encode[v % 85];
      v /= 85;
    }
    out += 5 - padding;
  }
  *out = 0;
}

/// Adjusts @a b and @a cb_b to account for possible ascii85 header and footer.
/// Note, the footer is only considered if a header is found.
bool
base85_header_footer_adjust (const char **b, size_t *cb_b)
{
  // Adjust for possible header.
  if (*cb_b > 1)
  {
    if ('<' == (*b)[0] && '~' == (*b)[1])
    {
      *b += 2;
      *cb_b -= 2;

      // Adjust for possible footer.
      if (*cb_b > 1)
      {
        if ('~' == (*b)[*cb_b - 2] && '>' == (*b)[*cb_b - 1])
        {
          *cb_b -= 2;
        }
      }
      return true;
    }
  }

  return false;
}

/// Decodes @a cb_b chatacters from @a b, and stores the result in @a out.
/// @pre @a b must contain at least @a cb_b bytes, and @a out must be large
/// enough to hold the result (including room for NUL terminator).
/// @return 0 for success, and the number of bytes written is returned
/// in @a out_cb.
///
/// @note Pass NULL for @a out to get the required byte count. 4 additional
/// bytes may be required for padding purposes.
int
base85_decode (const char *b, size_t cb_b, char *out, size_t *out_cb)
{
  static const unsigned int tbl[] = { 0x31c84b1, 0x95eed, 0x1c39, 0x55, 0x01 };

  if (!b || !out_cb)
    return -1;

  size_t padding = 0;

  // Number of bytes written to @a out.
  size_t cb = 0;

  while (base85_skipws (&b, &cb_b))
  {
    if ('z' == *b)
    {
      if (out)
      {
        memset (out, 0, 4);
        out += 4;
      }
      cb += 4;
      ++b;
      --cb_b;
      continue;
    }

    unsigned int v = 0;
    for (int c = 0; c < 5; ++c)
    {
      if (base85_skipws (&b, &cb_b))
      {
        if (!base85_decode_valid_char (*b))
          return -1;

        v += (*b++ - 33) * tbl[c];
        --cb_b;
      }
      else
      {
        // 0x54 = 'u' - 33
        v += 0x54 * tbl[c];
        ++padding;
      }
    }

    for (int c = 24; c >= 0; c -= 8)
    {
      if (out)
        *out++ = (v >> c) & 0xff;
      ++cb;
    }
  }

  *out_cb = cb - padding;
  return 0;
}

int
usage (const char *name)
{
  fprintf (stderr, "Usage: %s -e input | -d input\n", name);
  return 2;
}

int
main (int argc, char *argv[])
{
  if (3 != argc)
    return usage (argv[0]);

  const size_t input_len = strlen (argv[2]);

  if (!strcmp (argv[1], "-e"))
  {
    const size_t bufsize = base85_required_buffer_size (input_len);
    char *buffer = malloc (bufsize);
    if (!buffer)
      return 1;

    base85_encode (argv[2], input_len, buffer);
    printf ("<~%s~>\n", buffer);
    free (buffer);
  }
  else if (!strcmp (argv[1], "-d"))
  {
    size_t cb = 0;
    const char *input = argv[2];
    size_t input_cb = input_len;
    (void) base85_header_footer_adjust (&input, &input_cb);
    if (base85_decode (input, input_cb, NULL, &cb))
    {
      fprintf (stderr, "Decoding error\n");
      return 0;
    }
    char *buffer = malloc (cb + 4);
    if (!buffer)
      return 1;

    if (base85_decode (input, input_cb, buffer, &cb))
    {
      fprintf (stderr, "Unexpected decoding error\n");
      free (buffer);
      return 1;
    }

    (void) write (1, buffer, cb);

    free (buffer);
  }

  return 0;
}
