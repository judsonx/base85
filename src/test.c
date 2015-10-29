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

static b85_result_t
run_decode_test (const struct b85_test_t *entry)
{
  struct base85_context_t ctx;
  b85_result_t rv = B85_E_UNSPECIFIED;

  B85_TRY (B85_CONTEXT_INIT (&ctx))
  B85_TRY (B85_DECODE (entry->input_.b_, entry->input_.cb_b_, &ctx))
  B85_TRY (B85_DECODE_LAST (&ctx))

  size_t cb;
  char *out = B85_GET_OUTPUT (&ctx, &cb);
  B85_TRY (check_cb (cb, entry->expected_.cb_b_))
  B85_TRY (check_bytes (out, entry->expected_.b_, cb))

error_exit:
  B85_CONTEXT_DESTROY (&ctx);
  return rv;
}

static b85_result_t
run_encode_test (const struct b85_test_t *entry)
{
  struct base85_context_t ctx;
  struct base85_context_t ctx2 = { .out = NULL };

  b85_result_t rv = B85_E_UNSPECIFIED;
  char *out = NULL;
  B85_TRY (B85_CONTEXT_INIT (&ctx))
  B85_TRY (B85_CONTEXT_INIT (&ctx2))
  B85_TRY (B85_ENCODE (entry->input_.b_, entry->input_.cb_b_, &ctx))
  B85_TRY (B85_ENCODE_LAST (&ctx))

  size_t cb;
  out = B85_GET_OUTPUT (&ctx, &cb);
  B85_TRY (check_cb (cb, entry->expected_.cb_b_))
  size_t result_len = strlen (out);
  B85_TRY (check_cb (result_len, entry->expected_.cb_b_))
  B85_TRY (check_bytes (out, entry->expected_.b_, result_len))
  B85_TRY (B85_DECODE (out, result_len, &ctx2))
  B85_TRY (B85_DECODE_LAST (&ctx2))

  size_t cb2;
  out = B85_GET_OUTPUT (&ctx2, &cb2);
  B85_TRY (check_cb (cb2, entry->input_.cb_b_))
  B85_TRY (check_bytes (entry->input_.b_, out, cb2))

error_exit:
  B85_CONTEXT_DESTROY (&ctx);
  B85_CONTEXT_DESTROY (&ctx2);
  return rv;
}

static b85_result_t
b85_test_allbytes ()
{
  char input[256];
  for (size_t i = 0; i < 256; ++i)
    input[i] = (unsigned char) i;

  struct base85_context_t ctx;
  struct base85_context_t ctx2 = { .out = NULL };
  b85_result_t rv = B85_E_UNSPECIFIED;
  char *out = NULL;
  B85_TRY (B85_CONTEXT_INIT (&ctx))
  B85_TRY (B85_CONTEXT_INIT (&ctx2))
  B85_TRY (B85_ENCODE (input, 256, &ctx))
  B85_TRY (B85_ENCODE_LAST (&ctx))
  size_t cb;
  out = B85_GET_OUTPUT (&ctx, &cb);
  B85_TRY (B85_DECODE (out, cb, &ctx2))
  B85_TRY (B85_DECODE_LAST (&ctx2))

  size_t cb2;
  out = B85_GET_OUTPUT (&ctx2, &cb2);

  B85_TRY (check_cb (256, cb2))
  B85_TRY (check_bytes (input, out, cb2))

error_exit:
  B85_CONTEXT_DESTROY (&ctx);
  B85_CONTEXT_DESTROY (&ctx2);
  return rv;
}

static b85_result_t
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
  B85_TRY (B85_CONTEXT_INIT (&ctx))
  B85_TRY (B85_CONTEXT_INIT (&ctx2))

  size_t cb;
  for (size_t i = 0; i < N_ITERATIONS; ++i)
  {
    B85_TRY (B85_ENCODE (input, INPUT_CHUNK_SIZE, &ctx))
    out = B85_GET_OUTPUT (&ctx, &cb);

    B85_TRY (B85_DECODE (out, cb, &ctx2))
    out = B85_GET_OUTPUT (&ctx2, &cb);
    B85_TRY (check_cb (cb, INPUT_CHUNK_SIZE))
    B85_TRY (check_bytes (out, input, cb))
    B85_CLEAR_OUTPUT (&ctx);
    B85_CLEAR_OUTPUT (&ctx2);
  }
  B85_TRY (B85_ENCODE_LAST (&ctx))
  B85_TRY (B85_DECODE_LAST (&ctx2))

  // Sanity check on buffer size.
  if (ctx.out_cb > INPUT_CHUNK_SIZE * 2 || ctx2.out_cb > INPUT_CHUNK_SIZE * 2)
    rv = B85_E_UNSPECIFIED;

error_exit:
  B85_CONTEXT_DESTROY (&ctx);
  B85_CONTEXT_DESTROY (&ctx2);
  return rv;
}

static b85_result_t
b85_test_types ()
{
  // Some (most?) of the algorithms are assuming a 1 byte char.
  if ((sizeof (char) != 1) || (sizeof (unsigned char) != 1))
    return B85_E_UNSPECIFIED;
  return B85_E_OK;
}

#define B85_CREATE_TEST(name, test, input, input_cb, encoded, encoded_cb) \
static b85_result_t b85_test_##name () { \
  struct b85_test_t data = { { input, input_cb }, { encoded, encoded_cb } }; \
  return test (&data); \
}

#define B85_RUN_TEST(name, expected) do { \
  b85_result_t result = b85_test_##name (); \
  if (expected == result) \
  { \
    ++count; \
    printf ("  PASS -> %s\n", #name); \
  } \
  else \
  { \
    printf ( \
      "  FAIL -> %s (expected %s, got %s)\n", \
      #name, B85_DEBUG_ERROR_STRING (expected), \
      B85_DEBUG_ERROR_STRING (result) \
    ); \
  } \
  ++total; \
} while (0);

#define B85_RUN_EXPECT_SUCCESS(name) B85_RUN_TEST (name, B85_E_OK)

static const char helloworld[] = "hello world!";

B85_CREATE_TEST (s0, run_encode_test, helloworld, 0, "", 0)
B85_CREATE_TEST (s1, run_encode_test, helloworld, 1, "BE", 2)
B85_CREATE_TEST (s2, run_encode_test, helloworld, 2, "BOq", 3)
B85_CREATE_TEST (s3, run_encode_test, helloworld, 3, "BOtu", 4)
B85_CREATE_TEST (s4, run_encode_test, helloworld, 4, "BOu!r", 5)
B85_CREATE_TEST (s5, run_encode_test, helloworld, 5, "BOu!rDZ", 7)
B85_CREATE_TEST (s6, run_encode_test, helloworld, 6, "BOu!rD]f", 8)
B85_CREATE_TEST (s7, run_encode_test, helloworld, 7, "BOu!rD]j6", 9)
B85_CREATE_TEST (s8, run_encode_test, helloworld, 8, "BOu!rD]j7B", 10)
B85_CREATE_TEST (s9, run_encode_test, helloworld, 9, "BOu!rD]j7BEW", 12)
B85_CREATE_TEST (s10, run_encode_test, helloworld, 10, "BOu!rD]j7BEbk", 13)
B85_CREATE_TEST (s11, run_encode_test, helloworld, 11, "BOu!rD]j7BEbo7", 14)
B85_CREATE_TEST (s12, run_encode_test, helloworld, 12, "BOu!rD]j7BEbo80", 15)

B85_CREATE_TEST (h0, run_decode_test, "<~~>", 4, helloworld, 0)
B85_CREATE_TEST (h1, run_decode_test, "<~BE~>", 6, helloworld, 1)
B85_CREATE_TEST (h2, run_decode_test, "<~BOq~>", 7, helloworld, 2)
B85_CREATE_TEST (h3, run_decode_test, "<~BOtu~>", 8, helloworld, 3)
B85_CREATE_TEST (h4, run_decode_test, "<~BOu!r~>", 9, helloworld, 4)
B85_CREATE_TEST (h5, run_decode_test, "<~BOu!rDZ~>", 11, helloworld, 5)

B85_CREATE_TEST (h6, run_decode_test, " <~ ~> garbage", 14, helloworld, 0)
B85_CREATE_TEST (h7, run_decode_test, "<~BE~> ~~ <> xyz", 16, helloworld, 1)

// Starts out looking like a header, but isn't.
B85_CREATE_TEST (h8, run_decode_test, "<dSb1", 5, "V{oy", 4)

B85_CREATE_TEST (z1, run_encode_test, zeros, 4, "z", 1)
B85_CREATE_TEST (z2, run_encode_test, zeros, 8, "zz", 2)
B85_CREATE_TEST (z3, run_encode_test, zeros, 12, "zzz", 3)
B85_CREATE_TEST (z4, run_encode_test, zeros, 16, "zzzz", 4)
B85_CREATE_TEST (z5, run_encode_test, zeros, 20, "zzzzz", 5)
B85_CREATE_TEST (z6, run_encode_test, zeros, 24, "zzzzzz", 6)
B85_CREATE_TEST (z7, run_encode_test, zeros, 28, "zzzzzzz", 7)
B85_CREATE_TEST (z8, run_encode_test, zeros, 32, "zzzzzzzz", 8)

B85_CREATE_TEST (bin1, run_encode_test, binary1, 4,"s4IA0", 5)
B85_CREATE_TEST (bin2, run_encode_test, binary2, 4,"s8W-!", 5)

B85_CREATE_TEST (ws1, run_decode_test, " B\tOu  !rD]\nj7B\rEb o7  ", 23, helloworld, 11)
B85_CREATE_TEST (ws2, run_decode_test, "B\t\n\nOu!  rD]j7BEbo7\r\n", 21, helloworld, 11)

// Expect B85_E_INVALID_CHAR
B85_CREATE_TEST (f1, run_decode_test, "abcx", 4, "", 0)
// Expect B85_E_OVERFLOW
B85_CREATE_TEST (f2, run_decode_test, "s8W-\"", 5, "", 0)
// Expect B85_E_UNSPECIFIED
B85_CREATE_TEST (f3, run_decode_test, "BOu!r", 5, "xello", 5)
// Expect B85_E_INVALID_CHAR
B85_CREATE_TEST (f4, run_decode_test, "~>", 2, "", 0)
// Expect B85_E_BAD_FOOTER
B85_CREATE_TEST (f5, run_decode_test, "<~", 2, "", 0)
// Expect B85_E_BAD_FOOTER
B85_CREATE_TEST (f6, run_decode_test, "<~s4IA0 ", 7, "", 0)
// Expect B85_E_BAD_FOOTER
B85_CREATE_TEST (f7, run_decode_test, "<~~~>", 5, "", 0)
// Expect B85_E_BAD_FOOTER
B85_CREATE_TEST (f8, run_decode_test, "<~B~E~>", 7, "", 0)

int
run_tests (int argc, char *argv[])
{
  clock_t start, end;
  size_t count = 0;
  size_t total = 0;

  start = clock ();

  printf ("type sizes\n");
  B85_RUN_EXPECT_SUCCESS (types)

  printf ("small tests:\n");
  B85_RUN_EXPECT_SUCCESS (s0)
  B85_RUN_EXPECT_SUCCESS (s1)
  B85_RUN_EXPECT_SUCCESS (s2)
  B85_RUN_EXPECT_SUCCESS (s3)
  B85_RUN_EXPECT_SUCCESS (s4)
  B85_RUN_EXPECT_SUCCESS (s5)
  B85_RUN_EXPECT_SUCCESS (s6)
  B85_RUN_EXPECT_SUCCESS (s7)
  B85_RUN_EXPECT_SUCCESS (s8)
  B85_RUN_EXPECT_SUCCESS (s9)
  B85_RUN_EXPECT_SUCCESS (s10)
  B85_RUN_EXPECT_SUCCESS (s11)
  B85_RUN_EXPECT_SUCCESS (s12)
  B85_RUN_EXPECT_SUCCESS (z1)
  B85_RUN_EXPECT_SUCCESS (z2)
  B85_RUN_EXPECT_SUCCESS (z3)
  B85_RUN_EXPECT_SUCCESS (z4)
  B85_RUN_EXPECT_SUCCESS (z5)
  B85_RUN_EXPECT_SUCCESS (z6)
  B85_RUN_EXPECT_SUCCESS (z7)
  B85_RUN_EXPECT_SUCCESS (z8)
  B85_RUN_EXPECT_SUCCESS (bin1)
  B85_RUN_EXPECT_SUCCESS (bin2)

  printf ("header footer tests:\n");
  B85_RUN_EXPECT_SUCCESS (h0)
  B85_RUN_EXPECT_SUCCESS (h1)
  B85_RUN_EXPECT_SUCCESS (h2)
  B85_RUN_EXPECT_SUCCESS (h3)
  B85_RUN_EXPECT_SUCCESS (h4)
  B85_RUN_EXPECT_SUCCESS (h5)
  B85_RUN_EXPECT_SUCCESS (h6)
  B85_RUN_EXPECT_SUCCESS (h7)
  B85_RUN_EXPECT_SUCCESS (h8)

  printf ("decode whitespace:\n");
  B85_RUN_EXPECT_SUCCESS (ws1)
  B85_RUN_EXPECT_SUCCESS (ws2)

  printf ("all bytes:\n");
  B85_RUN_EXPECT_SUCCESS (allbytes)

  printf ("larger:\n");
  B85_RUN_EXPECT_SUCCESS (more_data)

  printf ("failure cases:\n");
  B85_RUN_TEST (f1, B85_E_INVALID_CHAR)
  B85_RUN_TEST (f2, B85_E_OVERFLOW)
  B85_RUN_TEST (f3, B85_E_UNSPECIFIED)
  B85_RUN_TEST (f4, B85_E_INVALID_CHAR)
  B85_RUN_TEST (f5, B85_E_BAD_FOOTER)
  B85_RUN_TEST (f6, B85_E_BAD_FOOTER)
  B85_RUN_TEST (f7, B85_E_BAD_FOOTER)
  B85_RUN_TEST (f8, B85_E_BAD_FOOTER)

  end = clock ();
  double elapsed = (double) (end - start) / CLOCKS_PER_SEC; 

  printf ("\n%zu TOTAL %zu FAILED in %.4fs \n", total, total - count, elapsed);

  if (total != count)
    return 1;
  return 0;
}

int
main (int argc, char *argv[])
{
  return run_tests (argc, argv);
}
