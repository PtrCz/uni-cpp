#ifndef UNI_CPP_IMPL_UNICODE_DATA_EMBED_SUPPORT_HPP
#define UNI_CPP_IMPL_UNICODE_DATA_EMBED_SUPPORT_HPP

#if defined(__cpp_pp_embed) && __cpp_pp_embed >= 202502L

#define UNI_CPP_IMPL_HAS_EMBED

#elif defined(__has_embed) && (defined(__clang__) || defined(__GNUC__))

#define UNI_CPP_IMPL_HAS_EMBED

#endif

#if defined(_MSVC_LANG)
#define UNI_CPP_IMPL_CPP_LANG _MSVC_LANG
#else
#define UNI_CPP_IMPL_CPP_LANG __cplusplus
#endif

#endif // UNI_CPP_IMPL_UNICODE_DATA_EMBED_SUPPORT_HPP