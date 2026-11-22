#ifndef UNI_CPP_IMPL_ORDINARY_LITERAL_ENCODING_HPP
#define UNI_CPP_IMPL_ORDINARY_LITERAL_ENCODING_HPP

#if ('A' == 65 && 'Z' == 90 && 'a' == 97 && 'z' == 122 && '0' == 48 && '9' == 57)

/// @brief If defined, the ordinary literal encoding is ASCII or an ASCII superset (such as UTF-8).
///
#define UNI_CPP_ASCII_IS_THE_ORDINARY_LITERAL_ENCODING

#endif

#endif // UNI_CPP_IMPL_ORDINARY_LITERAL_ENCODING_HPP