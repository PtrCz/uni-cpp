# `unicode_data_generator`

A Python tool used by the uni-cpp developers to generate Unicode tables and emit them as C++ code. For a given version of Unicode, it parses the Unicode Character Database (UCD) files, groups the code point properties into datasets, encodes and compresses the datasets, and finally, emits the encoded data tables as C++ code. The tool also provides dataset analysis and generation of test files.

## Usage

To use the `unicode_data_generator` tool, navigate to the `src` directory and use the following command:
```
python -m unicode_data_generator <arguments>
```

All of the generation output (and cache) gets saved to the directory this README is in, regardless of the working directory used, unless explicitly overridden with the `--output-dir` and `--cache-dir` options. It's most convenient to use the default directories.

---

To generate all C++ files and all test files, use:
```
python -m unicode_data_generator generate all --unicode-version <version>
```

The `<version>` argument can be either a semantic version (like `17.0.0`), or "`latest`". It's preferable to use a semantic version, otherwise, the generated files won't be stamped with a "`Unicode version: X.Y.Z`" comment.

Use the `--use-precomputed-tuning` option to save yourself some time, unless generating for a new Unicode version which doesn't have the hardcoded precomputed tuning yet.

It is possible to generate specific tables and tests as well:
```
python -m unicode_data_generator generate tables <"all" or dataset> --unicode-version <version>
python -m unicode_data_generator generate tests <"all" or test> --unicode-version <version>
```

To list available datasets and available tests, run:
```
python -m unicode_data_generator list datasets
python -m unicode_data_generator list tests
```

---

To generate an analysis of the datasets, run:
```
python -m unicode_data_generator analyze all --unicode-version <version>
```
or
```
python -m unicode_data_generator analyze <dataset> --unicode-version <version>
```

The analysis is printed out to the terminal and automatically saved to the output directory.

---

To see all available arguments and options, use the `--help` command. It gives a more detailed output for each subcommand (like `generate --help` vs. `generate tables --help`).

## The Generation Process

There are 1'114'112 valid Unicode code points, 1'112'064 valid Unicode scalar values. Storing a single boolean property for all of them would therefore take at least `1'112'064 bits = 139'008 bytes ≈ 139 KB` uncompressed. Clearly, storing the properties uncompressed is not ideal. Our goal is to compress the properties in a way that keeps the O(1) lookup time and still reduces the data size by a lot.

A problem with storing the Unicode properties is their representation format. Specifically, many properties are defined as enum values, strings, integers, booleans, and so on. That is a problem considering our end goal is to store them in pure C++ integer arrays. Clearly, there is a need for transforming all of these property values to integers. To do this, we group related Unicode properties that we want to generate together into _datasets_. The dataset defines how to transform all of the properties it groups into a single integer value. It is different for each dataset. It defines the format of the encoded integer value and how to transform the properties into that integer and back. Usually, it assigns given bits of the integer value to a specific property and then bit-packs all of the properties. Boolean properties are a single bit, integer properties occupy the space needed and enums are assigned integer values and then treated like integers. The dataset essentially defines an encoded value for each code point, which can be thought of as a key-value map, where the key is a code point, and the value is the encoded bit-packed property value.

> **Note:** The key-value map is brought up intentionally, because that is really what it is. In fact, the key does not necessarily need to be a code point at all, although it almost always is. `composition_mapping` is a dataset which generates a composition mapping, that is, it maps two code points to a single one, and the two code points are the query in this case. For this reason, the `composition_mapping` dataset uses a key consisting of two code points bit-packed next to each other, and a value of a single code point, which is the composition mapping of the two code points.

The most problematic property value transformation is for string-valued properties. These include properties like `uppercase_mapping`, `full_decomposition`, and similar. For these properties, the key-value map is not enough, because the encoded value must have the same format for each code point, meaning that bit-packing the uppercase mapping of length 1 for one code point and then bit-packing an uppercase mapping of length 2 for another code point will not work. Instead, the easiest solution is to just store all of the uppercase mappings in an extra array, and store indexes into that array along with the length of the mapping in the encoded value. This is just a single example of why datasets may need extra tables. There are datasets which do not need any, and datasets which need many extra tables. Overall, a dataset consists of a key-value map, and optionally, extra tables.

This solves the problem of property values not being integers, but the compression problem still needs a solution. Now, however, we have a common interface for datasets. At this stage we apply a mechanism that heavily compresses that key-value map, while keeping the O(1) lookup. The compression mechanism is called _an encoder_. There isn't a single encoder. Different encoders use different ways to compress the datasets. They take advantage of different patterns in the data, but only one encoder is applied for a given dataset. They cannot be chained. The output of an encoder is a set of arrays and a lookup function that can go through the arrays using a specific algorithm and determine the original encoded value for a given key.

The arrays generated by the encoder together with the extra arrays generated by the dataset (if any) are everything needed to look up any arbitrary code point (key) and get the encoded value, which then can be decoded to the original property values. All of this results in a nice, fast, compact storage of Unicode code point properties. 

The final stage is _the emitter_. It emits the C++ code files containing the generated arrays and a lookup function. The lookup function returns the value encoded by the dataset for a given code point (key). That value still needs to be decoded using the format the dataset specified to get the code point properties.

## Further Documentation

More detailed documentation about the dataset formats and encoder implementations can be found in the [uni-cpp/dev/docs](../../docs) directory.