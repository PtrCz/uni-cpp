#ifndef UNI_CPP_IMPL_EMBED_CODE
#error "Unpaired `end_embed_code.hpp`! Make sure to include `start_embed_code.hpp`!"
#endif

#undef UNI_CPP_IMPL_EMBED_CODE

#if UNI_CPP_IMPL_CPP_LANG == 202302L && defined(UNI_CPP_IMPL_HAS_EMBED)

#if defined(__clang__)

#pragma clang diagnostic pop

#elif defined(__GNUC__)

#pragma GCC diagnostic pop

#endif

#endif