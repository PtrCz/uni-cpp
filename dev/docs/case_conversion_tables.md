# Compact Unicode Case Mapping Lookup Tables

## Table of contents

- [Overview](#overview)
- [Definitions](#definitions)
- [Introduction](#introduction)
- [Storing Offsets for Simple Mappings](#storing-offsets-for-simple-mappings)
    - [Single Offset Table for All Case Types](#single-offset-table-for-all-case-types)
- [Compact Storage of Special Mappings](#compact-storage-of-special-mappings)
- [Putting It All Together](#putting-it-all-together)

<a name="overview"></a>
## Overview

This document explains the implementation of Unicode case mapping lookup tables, including the use of [multistage lookup tables](./multistage-lookup-tables.md) and offset-based storage.

<a name="definitions"></a>
## Definitions

In this document, there are two types of case mappings used:

- **Simple Mappings (1-to-1)**: These are mappings where one code point maps to exactly one other. For example, the lowercase letter `a` has an uppercase mapping to `A`. Another form of a simple mapping is when a code point has no defined case mapping; in such cases, it maps to itself. For example, the character `=` has a simple uppercase mapping to `=` (itself).
- **Special Mappings (1-to-N)**: These are mappings that expand one code point into multiple, such as German `ß` having an uppercase mapping to `SS`.

> **Note**: Context- or language-dependent mappings, such as the Turkish dotted and dotless I or the Greek final sigma, are represented in these tables by their context- and language-*independent* mapping equivalents.

<a name="introduction"></a>
## Introduction

When optimizing storage for Unicode properties, [multistage lookup tables](./multistage-lookup-tables.md) often are an effective solution. They are effective because Unicode tends to group code points with similar properties into contiguous ranges. However, case mappings vary widely and rarely repeat, so storing them directly in multistage lookup tables would lead to poor compression. This makes it necessary to take a more creative approach – storing mappings in a form that repeats for multiple code points. To achieve this, we store offsets instead of absolute target values for simple mappings.

<a name="storing-offsets-for-simple-mappings"></a>
## Storing Offsets for Simple Mappings

The key idea is to store an offset instead of the target code point. This offset is calculated as the difference between the target code point and the original code point. Offsets are well suited for storing simple mappings in a way that allows contiguous ranges of code points to share the same value representation.

Consider the ASCII letter range as an example. If we store the target code points directly, each letter maps to a unique value – `'a'` maps to `'A'`, `'b'` maps to `'B'`, and so on. However, by storing offsets instead, all ASCII letters share the same offset value; for instance, both `'a'` and `'b'` have an offset of 32. This allows for more efficient grouping and storage.

Another benefit of using offsets is the ability to represent code points without case mappings, such as symbols or punctuation, as simple mappings with an offset of zero. This approach avoids the need for special handling and allows them to be efficiently grouped and compressed.

<a name="single-offset-table-for-all-case-types"></a>
### Single Offset Table for All Case Types

In most cases, non-identity case mappings are symmetrical. That is, the original code point of a lowercase mapping is the uppercase mapping of the target code point, and vice versa. For example, in the ASCII range, the lowercase mapping of the character `'A'` (`U+0041`) is `'a'` (`U+0061`), and the uppercase mapping of the character `'a'` (`U+0061`) is `'A'` (`U+0041`).

For this reason, most simple case mappings have one offset for lowercase and casefold mappings and another one, which is an exact opposite of the former, for uppercase and titlecase mappings. In the example above, the lowercase mapping has an offset of `+32`, while the uppercase mapping has an offset of `-32`.

To reduce the size of the offset data, only one offset table is used. It stores offsets of the lowercase and casefold mappings and _negated_ offsets of uppercase and titlecase mappings. This deduplicates the symmetrical, opposite offsets of different case mapping types. Looking at the ASCII example, the `+32` lowercase mapping offset would be stored in the table, in contrast to the `-32` uppercase offset, which would not be stored, because its inverse is already in the table for the lowercase mapping.

When looking up an offset in the offset table, apply it as-is for lowercase and casefold mappings, and negate it for uppercase and titlecase mappings.

<a name="compact-storage-of-special-mappings"></a>
## Compact Storage of Special Mappings

The longest special case mapping defined for a single code point in Unicode is three code points long (as of Unicode 17.0.0). This allows us to take advantage of the fixed maximum length and store each special case mapping compactly using 64 bits.

Although Unicode code points are typically stored as 32-bit integers (when not encoded), only 21 bits are actually required to represent any valid code point. This means that storing the maximum of three code points requires 63 bits in total, using 21 bits per code point.

The length of the mapping must also be encoded alongside the code points. Since we know the mapping is special and cannot exceed three code points, its length must be either two or three. Because the length is either 2 or 3, it can be represented using a single bit. A value of `0` in the length bit indicates a mapping of 2 code points, while a value of `1` indicates 3 code points. This allows the actual length to be derived by simply adding 2 to the bit's value.

In total, 63 bits are used to store the code points and 1 bit is used to encode the mapping's length, perfectly fitting into 64 bits.

The following table illustrates the bit layout of the 64-bit value.

|      MSB     |      21-bits     |      21-bits     |      21-bits     |
|--------------|------------------|------------------|------------------|
| [length-bit] | [3rd code point] | [2nd code point] | [1st code point] |

<a name="putting-it-all-together"></a>
## Putting It All Together

To summarize, we have a table of simple mapping offsets, and a compact 64-bit format for storing special mappings. Building on that, we introduce an additional table containing unique special mappings of all case types.

In total, these two tables collectively represent all case mappings for every case type. The final piece is a mechanism to map any given code point to its corresponding case mapping.

For this, we use [multistage lookup tables](./multistage-lookup-tables.md). Since the multistage table is by far the largest among all the tables involved, it is desirable to use a single shared multistage lookup table for lowercase, uppercase, titlecase, and casefold lookups.

The multistage lookup tables map each code point to a value that encodes the case mapping information for all four case types: lowercase, uppercase, titlecase, and casefold. This value is divided into four 16-bit sections – one for each case type. Within each section, the most significant bit (MSB) serves as a flag indicating the type of the mapping: if the bit is set, the mapping is special; if it is unset, the mapping is simple. For simple mappings, the remaining 15 bits are used as an index into the shared offset table. For special mappings, those 15 bits are used as an index into the shared special mappings table. The format is illustrated in the following table:

<table style="text-align: center;">
  <thead>
    <tr>
      <th colspan="8">64 bits</th>
    </tr>
    <tr>
      <th colspan="2">Casefold<br>(16 bits)</th>
      <th colspan="2">Titlecase<br>(16 bits)</th>
      <th colspan="2">Uppercase<br>(16 bits)</th>
      <th colspan="2">Lowercase<br>(16 bits)</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><strong>MSB</strong><br>(Flag)</td>
      <td><strong>Index</strong><br>(15 bits)</td>
      <td><strong>MSB</strong><br>(Flag)</td>
      <td><strong>Index</strong><br>(15 bits)</td>
      <td><strong>MSB</strong><br>(Flag)</td>
      <td><strong>Index</strong><br>(15 bits)</td>
      <td><strong>MSB</strong><br>(Flag)</td>
      <td><strong>Index</strong><br>(15 bits)</td>
    </tr>
  </tbody>
</table>

The final optimization involves limiting the multistage lookup tables to encode values only for code points up to the highest code point that has a non-identity mapping for a given case type. During lookup, if the code point exceeds this maximum, the process can return early, indicating the code point maps to itself.