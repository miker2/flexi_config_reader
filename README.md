# Flexi Config Reader

Flexi Config Reader is a configuration language that employs a template-reference syntax
in order to express repetitive & structured configuration parameters in a more concise
manner. Usage of these constructs is optional. At it's core, the syntax uses concepts
similar to those found in json & toml: nested structures of key/value pairs. One can draw
parallels between some of the features found in this syntax and those made available by
the jinja templating python library.

## Core concepts

The syntax has three core concepts:

1. `struct` - This is a concrete entry of data. A `struct` may contain other `struct`s as
   well as key/value pairs.
2. `proto` - The `proto` keyword is used to define a template, which can be used to create
   a `struct`. A `proto` may contain other `struct`s, as well as keys who's values are
   represented by a variable
3. `reference` - The `reference` keyword is used to turn a `proto` into a concrete `struct`.
   The `proto` is `reference`d, given a name, and all contained variables are given a value.

In summary, a `proto` that is `reference`d, effectively becomes a `struct`.

## Other concepts

1. `include` syntax - existing configuration files may be included by other files. There
   are no restrictions around a `proto` being defined in one file, then `reference`d in
   another file. This highlights one of the advantages of this syntax: the ability to
   define a generic set of templates in one file, which will be use to multiple, repeated
   concrete structs with different names and parameters in a different file.
2. key-value reference - Much like bash, the syntax provides the ability to reference a
   previously defined value by its key, and assign it to another key.
3. Appended keys - While a `proto` defines a templated `struct`, one can add additional keys
   to the `proto` when it is referenced.
4. Fully qualified keys - One may define the configuration parameters using a combination
   of the tree-like structure found in json along with fully qualified key value pairs.
   These can not be mixed within the same file however.

While whitespace (except for following the `struct`, `proto`, `reference`, `as`  and `end` keywords)
is not significant it is convenient in order to better view the structure of the configuration parameters.

## Syntax details

### `struct` keyword

The `struct` keyword is used to define a structure of data, e.g.


```
struct foo

  key1 = 0
  key2 = 1.4
  key3 = "string"

  struct bar
    nested_key = 0
    baz = 3.14
  end bar

end foo
```

As mentioned above, a `struct` may contain key/value pairs as well as other `struct`, `proto`, or `reference` blocks.
When using the `struct` syntax, fully-qualified (or dotted) keys are not allowed within the struct. The above example
would result in the following fully-qualified key/value pairs:

```
foo.key1 = 0
foo.key2 = 1.4
foo.key3 = "string"
foo.bar.nested_key = 0
foo.bar.baz = 3.14
```

### `proto` keyword

The `proto` keyword is used to define a templated definition of a `struct`, e.g.

```
struct protos

  proto foo

    key1 = 0
    key2 = 1.4
    key3 = $KEY3

    struct bar
      ...
    end bar

  end foo

end protos
```

The above example defines a `proto` called "foo" with two concrete keys and a variable key `key3`. There is
also a nested struct (which may contain variable keys as well).

This `proto` does not add anything to the configuration parameters as is. It must be `reference`d in order to
turn it into a concrete `struct` of data.


### `reference` keyword

The `reference` keyword is used to turn a `proto` into a `struct`. Following on from the example above:

```
reference protos.foo as fuzz
  $KEY3 = "apple"
  +extra_key = 0x7FF
end fuzz
```

The above example introduces the following concepts:

1. When referencing a `proto`, the fully qualified name must be used (i.e. `protos.foo` instead of `foo`).
2. The variable `$KEY3` that was introduced in `protos.foo` is given a value when the `proto` is `reference`d.
3. `+extra_key` is an appended key. The `proto` "protos.foo" does not contain this key, but the resulting `struct fuzz`
   will contain this additional key.

The above `reference` could be defined as follows using the `struct` syntax:

```
struct fuzz
  key1 = 0
  key2 = 1.4
  key3 = "apple"
  extra_key = 0x7FF

  struct bar
    ...
  end bar
end fuzz
```

### Key-value references

As mentioned above, existing values may be referenced by their key in order to define a different key/value pair. E.g.

```
struct foo
  key1 = 0
  key2 = 1.4
end foo

struct bar
  key1 = 1
  key2 = $(foo.key2)
end bar
```

In this case, the key `bar.key2` would have the value of `foo.key2`, or in this case `1.4`. This construct can be
used within the `struct`, `proto` or `reference` constructs.


### Value types

The following value types are supported:

1. Decimal numbers
2. Floating-point numbers (standard or scientific notation)
3. Hexadecimal numbers (which resolve to a decimal number)
4. double-quoted string
5. List - a bracket-enclosed list of same-typed values from the types listed above (i.e. one may define a list
   of strings or floating-point numbers, but not a mixed combination of the two).

# Parsers

## Python
The syntax is defined in python using PEG notation. The `pe` library is used to parse the PEG defined grammar, which
is used to parse the configuration files. There is then a post-processing step to fully resolve all protos, references,
variable references, etc. The end result is a nested dictionary of key value pairs. There are a variety of tests and
examples which demonstrate more of the capabilities of the language.

## C++
The C++ implementation uses the [`taocpp::pegtl`](https://github.com/taocpp/PEGTL) library to define the grammar.
`PEGTL` uses a templatized syntax to define the grammar (which can be found [here](cpp/config_grammar.h)).  This is used
for parsing the raw config file, along with a set of [actions](cpp/config_actions.h) which define how to act on the parse
output.  Once the raw config files are parsed, there is a second pass that occurs in order to convert any `reference`s to
`proto`s into `struct`s as mentioned above.

`PEGTL` also provides some additional functionality to analyze the defined grammar and to generate a parse-tree from a
supplied configuration file.

### Dependencies

The following dependencies are required in order to compile the code:

 *  [`PEGTL`](https://github.com/taocpp/PEGTL) - The core library used for implementing the PEG-based parser
 *  [`magic_enum`](https://github.com/Neargye/magic_enum.git) - A header only library providing static reflection for enums
 *  [`{fmt}`](https://github.com/fmtlib/fmt.git) - A formatting library that provides an alternative to c stdio and C++ iostreams
 *  [`range-v3`](https://github.com/ericniebler/range-v3.git) - A range library for C++14/17/20
 *  [`googletest`](https://github.com/google/googletest.git) - The Google unit testing framework

All of these dependencies are automatically collected/installed via CMake `FetchContent`. Currently, there is no mechanism for using pre-installed versions.

### Build

This project is built using CMake. While there are a variety of ways to use cmake, these simple steps should lead to a successful build:

From the root of the source tree:
```
mkdir build
cd build
cmake ..
make
```

Or alternatively, using `ninja`:

```
mkdir build
cd build
cmake -G Ninja ..
ninja
```

### Tests

All C++-based tests can be found in the the [`tests`](tests) directory. Any new tests should be added here as well.
As mentioned above, the [Google testing framework](https://github.com/google/googletest) is used for testing. Once
the code is built, the tests can be run by executing `ctest` from the build directory. Refer to the
[`ctest`](https://cmake.org/cmake/help/latest/manual/ctest.1.html) documentation for further details. The tests can
also be run individually by executing the individual gtest binaries from the `tests` directory within your build
directory. See the googletest documentation for options.

### Examples

In addition to the tests, there are a number of simple applications that provide example code for the library usage.

 *  [`config_test`](cpp/config_test.cpp) - In the process of being converted to an actual unittest, this application executes a parsing run on a variety of [example config files](examples).
 *  [`config_build`](cpp/config_build.cpp) - This application can be used to parse a config file. Usage: `./cpp/config_reader ../example/config_example5.cfg`.
