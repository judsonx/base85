#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// Ascii85 alphabet.
static const unsigned char g_ascii85[] = {
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
      out[c] = g_ascii85[v % 85];
      v /= 85;
    }
    out += 5 - padding;
  }
  *out = 0;
}

int
main (int argc, char *argv[])
{
  if (2 != argc)
    return 2;

  char *buffer;

  const size_t input_len = strlen (argv[1]);
  const size_t bufsize = base85_required_buffer_size (input_len);

  buffer = malloc (bufsize);
  if (!buffer)
    return 1;

  base85_encode (argv[1], input_len, buffer);
  printf ("<~%s~>\n", buffer);

  free (buffer);

  return 0;
}
