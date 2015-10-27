#if !defined (BASE85_H__INCLUDED__)
#define BASE85_H__INCLUDED__

#include <stddef.h>

typedef enum
{
  B85_E_UNSPECIFIED = -1,
  B85_E_OK = 0,
  B85_E_OOM,
  B85_E_OVERFLOW,
  B85_E_INVALID_CHAR,
  B85_E_LOGIC_ERROR,
  B85_E_API_MISUSE,
  B85_E_END
} b85_result_t;

/// Translates @a val to an error string.
const char *
base85_error_string (b85_result_t val);

/// Context for the base85 decode functions.
struct base85_context_t
{
  /// Bytes "on deck" for encoding/decoding. Unsigned is important.
  unsigned char hold[5];

  /// The current hold position (i.e. how many bytes are currently in hold).
  size_t pos;

  /// Output buffer, memory is managed by the encode/decode functions.
  char *out;

  /// Current output position.
  char *out_pos;

  /// Number of bytes allocated for the out buffer.
  size_t out_cb;

  /// The total number of bytes processed as input.
  size_t processed;
};

/// Initializes a context object.
/// When done with the context, call base85_context_destroy().
b85_result_t
base85_context_init (struct base85_context_t *ctx);

/// Context cleanup. Frees memory associated with the context.
void
base85_context_destroy (struct base85_context_t *ctx);

/// Required buffer size to hold the base85 encoded result for an input string
/// with length @a input_size. Note: includes room for NUL.
size_t
base85_required_buffer_size (size_t input_size);

/// Encodes @a cb_b characters from @a b, and stores the result in @a ctx.
/// @pre @a b must contain at least @a cb_b bytes, and @a ctx must be a valid
/// context. 
///
/// Note: base85_encode_last() must be called in order to finalize the encode
/// operation.
b85_result_t
base85_encode (const char *b, size_t cb_b, struct base85_context_t *ctx);

/// Finalizes an encode operation that was initiated by calling base85_encode().
/// @pre @a ctx must be valid.
b85_result_t
base85_encode_last (struct base85_context_t *ctx);

/// Decodes @a cb_b characters from @a b. The result is stored in @a ctx.
/// @pre @a b must contain at least @a cb_b bytes, and @a ctx must be a valid
/// context.
///
/// Note: base85_decode_last() must be called in order to finalize the decode
/// operation.
///
/// @return 0 for success.
b85_result_t
base85_decode (
  const char *b, size_t cb_b, struct base85_context_t *ctx
);

/// Finalizes a decode operation that was initiated by calling base85_decode().
/// @pre @a ctx must be valid.
///
/// @return 0 for success.
b85_result_t
base85_decode_last (struct base85_context_t *ctx);

#endif // !defined (BASE85_H__INCLUDED__)
