#if !defined (BASE85_H__INCLUDED__)
#define BASE85_H__INCLUDED__

#include <stddef.h>

/// Base85 result values.
typedef enum
{
  /// Unspecified error. This value is not used by the library directly. It can
  /// be used by clients that wish to pass through b85_result_t values.
  B85_E_UNSPECIFIED = -1,

  /// Success.
  B85_E_OK = 0,

  /// Memory allocation failure.
  B85_E_BAD_ALLOC,

  /// Decoding a byte sequence resulted in an integer overflow.
  B85_E_OVERFLOW,

  /// An invalid byte was encountered (decoding).
  B85_E_INVALID_CHAR,

  /// The ascii85 footer is missing, or a rogue '~' byte was encountered.
  B85_E_BAD_FOOTER,

  /// Logic error in the library implementation.
  B85_E_LOGIC_ERROR,

  /// Indicates API misuse by a client.
  B85_E_API_MISUSE,

  /// End marker
  B85_E_END
} b85_result_t;

/// Tranlates @a val to a debug error string (i.e., "B85_E_OK").
const char *
base85_debug_error_string (b85_result_t val);

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

  /// Internal state (used for keeping track of the header/footer during
  /// decoding).
  unsigned char state;
};

/// Gets the output from @a ctx.
/// Returns the number of available bytes in @a cb.
/// @pre @a ctx is valid.
char *
base85_get_output (struct base85_context_t *ctx, size_t *cb);

/// Gets the number of input bytes processed by @a ctx.
size_t
base85_get_processed (struct base85_context_t *ctx);

/// Clears the output buffer in @a ctx. i.e. the next call to
/// base85_get_output() will return a byte count of zero.
/// @pre @a ctx is valid.
void
base85_clear_output (struct base85_context_t *ctx);

/// Initializes a context object.
/// When done with the context, call base85_context_destroy().
b85_result_t
base85_context_init (struct base85_context_t *ctx);

/// Resets an existing context, but does not free its memory. This is useful
/// for resetting the context before encoding/decoding a new data stream.
void
base85_context_reset (struct base85_context_t *ctx);

/// Context cleanup. Frees memory associated with the context.
void
base85_context_destroy (struct base85_context_t *ctx);

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
