#include "base85/base85.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define dimof(x) (sizeof(x) / sizeof (*x))

struct bytes_t
{
  const char *b_;
  size_t cb_b_;
};

struct b85_test_t
{
  struct bytes_t input_;
  struct bytes_t expected_;
};

static const char zeros[32];

static const struct b85_test_t g_small_tests[] = {
  { { "h", 1 }, { "BE", 2 } },
  { { "he", 2 }, { "BOq", 3 } },
  { { "hel", 3 }, { "BOtu", 4 } },
  { { "hell", 4 }, { "BOu!r", 5 } },
  { { "hello", 5 }, { "BOu!rDZ", 7 } },
  { { "hello ", 6 }, { "BOu!rD]f", 8 } },
  { { "hello w", 7 }, { "BOu!rD]j6", 9 } },
  { { "hello wo", 8 }, { "BOu!rD]j7B", 10 } },
  { { "hello wor", 9 }, { "BOu!rD]j7BEW", 12 } },
  { { "hello worl", 10 }, { "BOu!rD]j7BEbk", 13 } },
  { { "hello world", 11 }, { "BOu!rD]j7BEbo7", 14 } },
  { { "hello world!", 12 }, { "BOu!rD]j7BEbo80", 15 } },

  { { zeros, 4 }, { "z", 1 } },
  { { zeros, 8 }, { "zz", 2 } },
  { { zeros, 12 }, { "zzz", 3 } },
  { { zeros, 16 }, { "zzzz", 4 } },
  { { zeros, 20 }, { "zzzzz", 5 } },
  { { zeros, 24 }, { "zzzzzz", 6 } },
  { { zeros, 28 }, { "zzzzzzz", 7 } },
  { { zeros, 32 }, { "zzzzzzzz", 8 } },
};

static const struct b85_test_t g_whitespace_tests[] = {
  { { "hello world", 11 }, { " B\tOu  !rD]\nj7B\rEb o7  ", 23 } },
  { { "hello world", 11 }, { "B\t\n\nOu!  rD]j7BEbo7\r\n", 21 } },
};

int
run_ws_test (const struct b85_test_t *entry)
{
  size_t cb;
  if (base85_decode (entry->expected_.b_, entry->expected_.cb_b_, NULL, &cb))
    return -1;

  if (cb != entry->input_.cb_b_)
    return -1;

  char *buffer = malloc (cb + 4);

  size_t cb2;
  if (base85_decode (entry->expected_.b_, entry->expected_.cb_b_, buffer, &cb2))
    return -1;

  if (cb != cb2)
    return -1;

  if (memcmp (buffer, entry->input_.b_, cb2))
    return -1;

  return 0;
}

int
run_small_test (const struct b85_test_t *entry)
{
  size_t required_bytes = base85_required_buffer_size (entry->input_.cb_b_);
  char *buffer2 = NULL;
  char *buffer = malloc (required_bytes);
  if (!buffer)
    return -1;

  base85_encode (entry->input_.b_, entry->input_.cb_b_, buffer);
  size_t result_len = strlen (buffer);
  if (result_len != entry->expected_.cb_b_)
    goto error_exit;

  if (result_len >= required_bytes)
    goto error_exit;

  if (memcmp (buffer, entry->expected_.b_, result_len))
    goto error_exit;

  size_t cb;
  if (base85_decode (buffer, result_len, NULL, &cb))
    goto error_exit;

  buffer2 = malloc (cb + 4);
  if (!buffer2)
    goto error_exit;

  size_t cb2;
  if (base85_decode (buffer, result_len, buffer2, &cb2))
    goto error_exit;

  if (cb2 != cb)
    goto error_exit;

  if (cb != entry->input_.cb_b_)
    goto error_exit;

  if (memcmp (entry->input_.b_, buffer2, cb))
    goto error_exit;

  return 0;

  error_exit:
    free (buffer);
    free (buffer2);
    return -1;
}

int
run_small_tests ()
{
  printf ("small tests: ");
  size_t count = 0;
  for (size_t i = 0; i < dimof (g_small_tests); ++i)
  {
    if (run_small_test (&g_small_tests[i]))
      break;

    ++count;
  }

  if (dimof (g_small_tests) != count)
  {
    printf ("FAIL [%zu]\n", count);
    return -1;
  }

  printf ("PASS [%zu]\n", count);
  return 0;
}

int
run_ws_tests ()
{
  printf ("decode whitespace: ");
  size_t count = 0;
  for (size_t i = 0; i < dimof (g_whitespace_tests); ++i)
  {
    if (run_ws_test (&g_whitespace_tests[i]))
      break;

    ++count;
  }

  if (dimof (g_whitespace_tests) != count)
  {
    printf ("FAIL [%zu]\n", count);
    return -1;
  }

  printf ("PASS [%zu]\n", count);
  return 0;
}

int
run_tests (int argc, char *argv[])
{
  if (run_small_tests ())
    return 1;
  if (run_ws_tests ())
    return 1;

  return 0;
}

int
main (int argc, char *argv[])
{
  return run_tests (argc, argv);
}
