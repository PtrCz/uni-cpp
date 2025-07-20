# Multistage Lookup Tables

## Table of contents

- [Overview](#overview)
- [Extra Optimizations](#extra-optimizations)
    - [Optimization Considerations](#optimization-considerations)
- [Lookup algorithm](#lookup-algorithm)
    - [Step-by-Step](#step-by-step)
    - [Minimal Python implementation](#minimal-python-implementation)
- [Applications](#applications)

<a name="overview"></a>
## Overview

A multistage lookup table is a data structure used for efficient storage and retrieval of Unicode properties. To get a basic understanding of this data structure read [Fast Lookup of Unicode Properties](https://here-be-braces.com/fast-lookup-of-unicode-properties/).

<a name="extra-optimizations"></a>
## Extra Optimizations

Extra optimizations implemented in this library to improve the storage efficiency:

1. **Block Overlapping**:
    Overlap the blocks as much as possible to save _a lot of_ space in the `stage2` table.

2. **Unique Offsets table**:
    Stores unique offsets to the `stage2` table in an extra table when beneficial. This often allows the `stage1` table to use a smaller value type, while the extra table using the larger value type has significantly fewer values.

3. **In-place Property Storage**:
    Stores properties directly in the `stage2` table when beneficial. It often eliminates the need for a redundant `stage3` table.

4. **Fine-tuning the block size parameter**:
    Use fine-tuned block size parameter instead of always using the same block size for different tables.

<a name="optimization-considerations"></a>
### Optimization Considerations

The _Block Overlapping_ optimization is applied **unconditionally** (as it practically always saves space), while the _Unique Offsets table_ and the _In-place Property Storage_ optimizations are **only** applied when they actually save space. The optimal combination of these techniques is determined during table generation, based on the input data.

<a name="lookup-algorithm"></a>
## Lookup algorithm

The lookup algorithm depends on the chosen optimizations and block size, which are selected during table generation and fixed afterward. In compiled languages (like C++), where these parameters are known at compile time, the algorithm becomes **branch-free**, with all conditions resolved statically.

In this documentation:
- The integer `code_point` refers to the Unicode code point for which the lookup is performed.
- The integer `block_size` refers to the number of code points that were grouped into each block during table generation.
- The boolean flag `stage1_needs_extra_lookup` indicates whether **Optimization 2** (_Unique Offsets table_) was applied.
- The boolean flag `stage2_holds_properties_inplace` indicates whether **Optimization 3** (_In-place Property Storage_) was applied.
- The names `stage1`, `stage2_offsets`, `stage2` and `stage3` each refer to the generated table with the corresponding name.

<a name="step-by-step"></a>
### Step-by-Step:

1. **Get Stage 1 Value**
    Start by dividing the `code_point` by the `block_size` using integer (floor) division. This gives you the index into the first table, `stage1`. Use this index to retrieve the corresponding value from the `stage1` table.

2. **Resolve Stage 2 Offset**
    Depending on the`stage1_needs_extra_lookup` flag value, interpret the value from `stage1` in one of two ways:

    - If the flag is `true`, treat the value as an index into the `stage2_offsets` table and use it to get the actual offset.
    - If the flag is `false`, use the value from `stage1` directly as the offset.

3. **Get Stage 2 Value**
    Determine the position of the code point within its block by computing the remainder of the division (`code_point % block_size`). Add this to the previously resolved offset to get the index into the `stage2` table. Use this index to retrieve the value from the `stage2` table at that position.

4. **Resolve Final Property**
    Finally, check whether properties are stored directly in the `stage2` table, as indicated by the `stage2_holds_properties_inplace` flag.

    - If the flag is `true`, the value from `stage2` is the final property.
    - If the flag is `false`, the value is an index into the `stage3` table, where the actual property is stored.

<a name="minimal-python-implementation"></a>
### Minimal Python implementation

```python
# STEP 1
stage1_index = code_point // block_size # `//` performs integer (floor) division
stage1_value = stage1[stage1_index]

# STEP 2
offset = stage2_offsets[stage1_value] if stage1_needs_extra_lookup else stage1_value

# STEP 3
stage2_index = offset + code_point % block_size
stage2_value = stage2[stage2_index]

# STEP 4
return stage2_value if stage2_holds_properties_inplace else stage3[stage2_value]
```

<a name="applications"></a>
## Applications

Multistage Lookup Tables are used in this library to store and retrieve:
- Character case mappings. For more details read [this](./case_conversion_tables.md).