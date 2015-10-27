#include "base85/base85.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
static const char binary1[] = { 0xff, 0xd8, 0xff, 0xe0 };
static const char binary2[] = { 0xff, 0xff, 0xff, 0xff };

#define B85_TRY(func) do { rv = func; if (rv) goto error_exit; } while (0);

static b85_result_t
check_cb (size_t cb0, size_t cb1)
{
  return cb0 == cb1 ? B85_E_OK : B85_E_UNSPECIFIED;
}

static b85_result_t
check_bytes (const void *b0, const void *b1, size_t cb)
{
  if (memcmp (b0, b1, cb))
    return B85_E_UNSPECIFIED;
  return B85_E_OK;
}

static bool
run_ws_test (const struct b85_test_t *entry)
{
  struct base85_context_t ctx;
  b85_result_t rv = B85_E_UNSPECIFIED;

  B85_TRY (base85_context_init (&ctx))
  B85_TRY (base85_decode (entry->expected_.b_, entry->expected_.cb_b_, &ctx))
  B85_TRY (base85_decode_last (&ctx))

  size_t cb;
  char *out = base85_get_output (&ctx, &cb);
  B85_TRY (check_cb (cb, entry->input_.cb_b_))
  B85_TRY (check_bytes (out, entry->input_.b_, cb))

  error_exit:

  base85_context_destroy (&ctx);
  return B85_E_OK == rv;
}

static bool
run_small_test (const struct b85_test_t *entry)
{
  struct base85_context_t ctx;
  struct base85_context_t ctx2 = { .out = NULL };

  b85_result_t rv = B85_E_UNSPECIFIED;
  char *out = NULL;
  B85_TRY (base85_context_init (&ctx))
  B85_TRY (base85_context_init (&ctx2))
  B85_TRY (base85_encode (entry->input_.b_, entry->input_.cb_b_, &ctx))
  B85_TRY (base85_encode_last (&ctx))

  size_t cb;
  out = base85_get_output (&ctx, &cb);
  B85_TRY (check_cb (cb, entry->expected_.cb_b_))
  size_t result_len = strlen (out);
  B85_TRY (check_cb (result_len, entry->expected_.cb_b_))
  B85_TRY (check_bytes (out, entry->expected_.b_, result_len))
  B85_TRY (base85_decode (out, result_len, &ctx2))
  B85_TRY (base85_decode_last (&ctx2))

  size_t cb2;
  out = base85_get_output (&ctx2, &cb2);
  B85_TRY (check_cb (cb2, entry->input_.cb_b_))
  B85_TRY (check_bytes (entry->input_.b_, out, cb2))

  error_exit:

  base85_context_destroy (&ctx);
  base85_context_destroy (&ctx2);
  return B85_E_OK == rv;
}

static bool
b85_test_allbytes ()
{
  char input[256];
  for (size_t i = 0; i < 256; ++i)
    input[i] = (unsigned char) i;

  struct base85_context_t ctx;
  struct base85_context_t ctx2 = { .out = NULL };
  b85_result_t rv = B85_E_UNSPECIFIED;
  char *out = NULL;
  B85_TRY (base85_context_init (&ctx))
  B85_TRY (base85_context_init (&ctx2))
  B85_TRY (base85_encode (input, 256, &ctx))
  B85_TRY (base85_encode_last (&ctx))
  size_t cb;
  out = base85_get_output (&ctx, &cb);
  B85_TRY (base85_decode (out, cb, &ctx2))
  B85_TRY (base85_decode_last (&ctx2))

  size_t cb2;
  out = base85_get_output (&ctx2, &cb2);

  B85_TRY (check_cb (256, cb2))
  B85_TRY (check_bytes (input, out, cb2))

error_exit:
  base85_context_destroy (&ctx);
  base85_context_destroy (&ctx2);
  return B85_E_OK == rv;
}

static bool
b85_test_more_data ()
{
  static const size_t INPUT_CHUNK_SIZE = 2048;
  static const size_t N_ITERATIONS = 2000;

  char input[INPUT_CHUNK_SIZE];
  for (size_t i = 0; i < INPUT_CHUNK_SIZE; ++i)
    input[i] = (unsigned char) i;

  struct base85_context_t ctx;
  struct base85_context_t ctx2 = { .out = NULL };
  b85_result_t rv = B85_E_UNSPECIFIED;
  char *out = NULL;
  B85_TRY (base85_context_init (&ctx))
  B85_TRY (base85_context_init (&ctx2))

  size_t cb;
  for (size_t i = 0; i < N_ITERATIONS; ++i)
  {
    B85_TRY (base85_encode (input, INPUT_CHUNK_SIZE, &ctx))
    out = base85_get_output (&ctx, &cb);

    B85_TRY (base85_decode (out, cb, &ctx2))
    out = base85_get_output (&ctx2, &cb);
    B85_TRY (check_cb (cb, INPUT_CHUNK_SIZE))
    B85_TRY (check_bytes (out, input, cb))
    base85_clear_output (&ctx);
    base85_clear_output (&ctx2);
  }
  B85_TRY (base85_encode_last (&ctx))
  B85_TRY (base85_decode_last (&ctx2))

  // Sanity check on buffer size.
  if (ctx.out_cb > INPUT_CHUNK_SIZE * 2 || ctx2.out_cb > INPUT_CHUNK_SIZE * 2)
    rv = B85_E_UNSPECIFIED;

error_exit:
  base85_context_destroy (&ctx);
  base85_context_destroy (&ctx2);
  return B85_E_OK == rv;
}

#define B85_CREATE_TEST(name, test, input, input_cb, encoded, encoded_cb) \
static bool b85_test_##name () { \
  struct b85_test_t data = { { input, input_cb }, { encoded, encoded_cb } }; \
  return test (&data); \
}

#define B85_RUN_TEST(name) do { \
  if (b85_test_##name()) \
  { \
    ++count; \
    printf ("  PASS :: %s\n", #name); \
  } \
  else \
  { \
    printf ("  FAIL :: %s\n", #name); \
  } \
  ++total; \
} while (0);


B85_CREATE_TEST (s1, run_small_test, "h", 1, "BE", 2)
B85_CREATE_TEST (s2, run_small_test, "he", 2, "BOq", 3)
B85_CREATE_TEST (s3, run_small_test, "hel", 3, "BOtu", 4)
B85_CREATE_TEST (s4, run_small_test, "hell", 4, "BOu!r", 5)
B85_CREATE_TEST (s5, run_small_test, "hello", 5, "BOu!rDZ", 7)
B85_CREATE_TEST (s6, run_small_test, "hello ", 6, "BOu!rD]f", 8)
B85_CREATE_TEST (s7, run_small_test, "hello w", 7, "BOu!rD]j6", 9)
B85_CREATE_TEST (s8, run_small_test, "hello wo", 8, "BOu!rD]j7B", 10)
B85_CREATE_TEST (s9, run_small_test, "hello wor", 9, "BOu!rD]j7BEW", 12)
B85_CREATE_TEST (s10, run_small_test, "hello worl", 10, "BOu!rD]j7BEbk", 13)
B85_CREATE_TEST (s11, run_small_test, "hello world", 11, "BOu!rD]j7BEbo7", 14)
B85_CREATE_TEST (s12, run_small_test, "hello world!", 12, "BOu!rD]j7BEbo80", 15)

B85_CREATE_TEST (z1, run_small_test, zeros, 4, "z", 1)
B85_CREATE_TEST (z2, run_small_test, zeros, 8, "zz", 2)
B85_CREATE_TEST (z3, run_small_test, zeros, 12, "zzz", 3)
B85_CREATE_TEST (z4, run_small_test, zeros, 16, "zzzz", 4)
B85_CREATE_TEST (z5, run_small_test, zeros, 20, "zzzzz", 5)
B85_CREATE_TEST (z6, run_small_test, zeros, 24, "zzzzzz", 6)
B85_CREATE_TEST (z7, run_small_test, zeros, 28, "zzzzzzz", 7)
B85_CREATE_TEST (z8, run_small_test, zeros, 32, "zzzzzzzz", 8)

B85_CREATE_TEST (bin1, run_small_test, binary1, 4,"s4IA0", 5)
B85_CREATE_TEST (bin2, run_small_test, binary2, 4,"s8W-!", 5)

B85_CREATE_TEST (ws1, run_ws_test, "hello world", 11, " B\tOu  !rD]\nj7B\rEb o7  ", 23)
B85_CREATE_TEST (ws2, run_ws_test, "hello world", 11, "B\t\n\nOu!  rD]j7BEbo7\r\n", 21)

int
run_tests (int argc, char *argv[])
{
  clock_t start, end;
  size_t count = 0;
  size_t total = 0;

  start = clock ();

  printf ("small tests:\n");
  B85_RUN_TEST (s1);
  B85_RUN_TEST (s2);
  B85_RUN_TEST (s3);
  B85_RUN_TEST (s4);
  B85_RUN_TEST (s5);
  B85_RUN_TEST (s6);
  B85_RUN_TEST (s7);
  B85_RUN_TEST (s8);
  B85_RUN_TEST (s9);
  B85_RUN_TEST (s10);
  B85_RUN_TEST (s11);
  B85_RUN_TEST (s12);
  B85_RUN_TEST (z1);
  B85_RUN_TEST (z2);
  B85_RUN_TEST (z3);
  B85_RUN_TEST (z4);
  B85_RUN_TEST (z5);
  B85_RUN_TEST (z6);
  B85_RUN_TEST (z7);
  B85_RUN_TEST (z8);
  B85_RUN_TEST (bin1);
  B85_RUN_TEST (bin2);

  printf ("decode whitespace:\n");
  B85_RUN_TEST (ws1);
  B85_RUN_TEST (ws2);

  printf ("all bytes:\n");
  B85_RUN_TEST (allbytes);

  printf ("larger\n");
  B85_RUN_TEST (more_data);

  end = clock ();
  double elapsed = (double) (end - start) / CLOCKS_PER_SEC; 

  printf ("\nTOTAL: %zu FAILED: %zu in %.4fs \n", total, total - count, elapsed);

  if (total != count)
    return 1;
  return 0;
}

int
main (int argc, char *argv[])
{
  return run_tests (argc, argv);
}
