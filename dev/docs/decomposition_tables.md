# The `decomposition` Dataset IR Format

## Overview

This dataset encodes Unicode decomposition information into a single packed integer per code point. It covers full canonical decomposition, full compatibility decomposition, and the decomposition type of every code point, excluding precomposed Hangul syllables. Precomposed Hangul syllable decomposition data is not stored in the encoded tables, because it can be arithmetically computed at runtime. Storing it explicitly in the tables would significantly increase the data size while not providing any additional benefits. Accordingly, all precomposed Hangul syllables are represented by the default value, `0`.

## Format

### Canonical and Compatibility Decompositions

Both canonical and compatibility decompositions use the same, shared format. It is a 19-bit value with the following structure:

<table style="text-align: center;">
  <thead>
    <tr>
      <th colspan="2">19 bits</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><strong>Length</strong><br>(5 bits)</td>
      <td><strong>Index</strong><br>(14 bits)</td>
    </tr>
  </tbody>
</table>

Length is the length of the decomposition. A length of `0` represents an identity decomposition, meaning the code point maps to itself (unless it is a precomposed Hangul syllable). This is possible, because the decomposition of a code point cannot be an empty string.

The index is an offset into a flattened code point array containing every unique canonical and compatibility decomposition. For a non-zero length, the decomposition is defined as the following list slice: `mappings_array[index : index + length]`.

### Decomposition Type

The decomposition type of a code point is encoded using a 5-bit value. It is simply an index into the following array of possible decomposition type values:

```python
[None, 'font', 'noBreak', 'initial', 'medial', 'final', 'isolated', 'circle',
'super', 'sub', 'vertical', 'wide', 'narrow', 'small', 'square', 'fraction', 'compat']
```

If no decomposition type is defined, the value is `None`, encoded as `0`.

### Putting It Together

The final encoded value is obtained by bit-packing these three fields into a single integer:

```python
value = (decomposition_type << 38) | (compatibility << 19) | canonical
```

The final value is illustrated in the following table:

<table style="text-align: center;">
  <thead>
    <tr>
      <th colspan="8">64 bits</th>
    </tr>
    <tr>
      <th colspan="2">Unused<br>(21 bits)</th>
      <th colspan="2">Decomposition Type<br>(5 bits)</th>
      <th colspan="2">Full Compatibility Decomposition<br>(19 bits)</th>
      <th colspan="2">Full Canonical Decomposition<br>(19 bits)</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td colspan="2"></td>
      <td colspan="2">Enum value encoded as an integer</td>
      <td><strong>Length</strong><br>(5 bits)</td>
      <td><strong>Index</strong><br>(14 bits)</td>
      <td><strong>Length</strong><br>(5 bits)</td>
      <td><strong>Index</strong><br>(14 bits)</td>
    </tr>
  </tbody>
</table>