#if !defined (BASE85_H__INCLUDED__)
#define BASE85_H__INCLUDED__

#include <stddef.h>

/// Context for the base85 decode functions.
struct base85_decode_context_t
{
  /// Bytes "on deck" for decoding.
  char hold[5];

  /// The current hold position (i.e. how many bytes are currently in hold.
  size_t pos;

  /// Output buffer, memory is managed by the decode functions.
  char *out;

  /// Current output position.
  char *out_pos;

  /// Number of bytes allocated for the out buffer.
  size_t out_cb;
};

/// Initializes a context object.
/// When done with the context, call base85_decode_context_destroy().
int
base85_decode_context_init (struct base85_decode_context_t *ctx);

/// Context cleanup. Frees memory associated with the context.
void
base85_decode_context_destroy (struct base85_decode_context_t *ctx);

/// Required buffer size to hold the base85 encoded result for an input string
/// with length @a input_size. Note: includes room for NUL.
size_t
base85_required_buffer_size (size_t input_size);

/// Encodes @a cb_b characters from @a b, and stores the result in @a out.
/// @pre @a b must contain at least @a cb_b bytes, and @a out must be large
/// enough to hold the result (including room for NUL terminator). 
void
base85_encode (const char *b, size_t cb_b, char *out);

/// Decodes @a cb_b characters from @a b. The result is stored in @a ctx.
/// @pre @a b must contain at least @a cb_b bytes, and @a ctx must be a valid
/// decode context.
///
/// Note: base85_decode_last() must be called in order to finalize the decode
/// operation.
///
/// @return 0 for success.
int
base85_decode (
  const char *b, size_t cb_b, struct base85_decode_context_t *ctx
);

/// Finalizes a decode operation that was initiated by calling base85_decode().
/// @pre @a ctx must be valid.
///
/// @return 0 for success.
int
base85_decode_last (struct base85_decode_context_t *ctx);

#endif // !defined (BASE85_H__INCLUDED__)
