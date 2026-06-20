#ifdef UNI_CPP_IMPL_EMBED_CODE
#error "Unpaired `start_embed_code.hpp`! Make sure to include `end_embed_code.hpp`!"
#endif

#define UNI_CPP_IMPL_EMBED_CODE

#include "support.hpp"

#if UNI_CPP_IMPL_CPP_LANG == 202302L && defined(UNI_CPP_IMPL_HAS_EMBED)

#if defined(__clang__)

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc23-extensions"

#elif defined(__GNUC__)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc++26-extensions"

#endif

#endif