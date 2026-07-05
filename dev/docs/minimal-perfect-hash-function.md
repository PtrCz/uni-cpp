# Minimal Perfect Hash Function

<a name="overview"></a>
## Overview

A minimal perfect hash function (MPHF) is a hash function designed for a fixed set of 𝑛 distinct keys that maps each key to a unique integer in the range [0, 𝑛 − 1]. It does not need to preserve the order of the keys, but it must be collision-free. In practice, an MPHF is useful only if it requires little space while supporting constant-time evaluation; otherwise, it offers little advantage over a hash table that explicitly stores the mapping from each key to its assigned integer.

<a name="implementation"></a>
## Implementation

Over the years, there have been many general-purpose MPHFs developed. They differ by complexity, construction time, lookup time, and data size. For us, a simpler, older implementation is sufficient. The one we use is based on Steve Hanov's blog post, [Throw away the keys: Easy, Minimal Perfect Hashing](https://stevehanov.ca/blog/throw-away-the-keys-easy-minimal-perfect-hashing), which itself is based on _Practical Minimal Perfect Hash Functions for Large Databases_ (CACM, 35(1):105-121). We use the construction algorithm presented in Hanov's article. Our data structure represents a key-value map in which every unspecified key maps to a default value, while a fixed subset of keys maps to explicit values. We construct an MPHF over the subset of non-default keys and generate an array indexed by the resulting hash values in the range [0, 𝑛 − 1]. Each entry stores both the corresponding key and its associated value.

<a name="arbitrary-key-lookup"></a>
### Arbitrary Key Lookup

Storing the keys alongside the values makes it possible to look up arbitrary keys, not just the subset of non-default keys over which the MPHF was constructed. To perform an arbitrary key lookup, the queried key is first hashed using the MPHF to an index. Then, the key stored at that index is compared with the queried key. If they match, the associated value is returned; otherwise, the queried key is not part of the encoded set, and the default value is returned.

<a name="applications"></a>
## Applications

This library uses a minimal perfect hash function to efficiently store and retrieve the Unicode code point composition data.