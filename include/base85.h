#if !defined (BASE85_H__INCLUDED__)
#define BASE85_H__INCLUDED__

#include <string.h>

/// Required buffer size to hold the base85 encoded result for an input string
/// with length @a input_size. Note: includes room for NUL.
size_t
base85_required_buffer_size (size_t input_size);

/// Encodes @a cb_b characters from @a b, and stores the result in @a out.
/// @pre @a b must contain at least @a cb_b bytes, and @a out must be large
/// enough to hold the result (including room for NUL terminator). 
void
base85_encode (const char *b, size_t cb_b, char *out);

/// Decodes @a cb_b characters from @a b, and stores the result in @a out.
/// @pre @a b must contain at least @a cb_b bytes, and @a out must be large
/// enough to hold the result (including room for NUL terminator).
/// @return 0 for success, and the number of bytes written is returned
/// in @a out_cb.
///
/// @note Pass NULL for @a out to get the required byte count. 4 additional
/// bytes may be required for padding purposes.
int
base85_decode (const char *b, size_t cb_b, char *out, size_t *out_cb);

#endif // !defined (BASE85_H__INCLUDED__)
