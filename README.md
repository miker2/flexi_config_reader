# Flexi Config Reader

Flexi Config Reader is a configuration language that employs a template-reference syntax
in order to express repetitive & structured configuration parameters in a more concise
manner. Usage of these constructs is optional. At its core, the syntax uses concepts
similar to those found in json & toml: nested structures of key/value pairs. One can draw
parallels between some of the features found in this syntax and those made available by
the jinja templating python library.

## Core concepts

The syntax has three core concepts:

1. [`struct`](#struct-keyword) - This is a concrete entry of data. A `struct` may contain other `struct`s as
   well as key/value pairs.
2. [`proto`](#proto-keyword) - The `proto` keyword is used to define a template, which can be used to create
   a `struct`. A `proto` may contain other `struct`s, `reference`s (see below), as well as
   key/value pairs. The values in a proto (and all sub-objects) may be represented by a variable.
3. [`reference`](#reference-keyword) - The `reference` keyword is used to turn a `proto` into a concrete
   `struct`. The `proto` is `reference`d, given a name, and all contained variables are given a value.

In summary, a `proto` that is `reference`d, effectively becomes a `struct`.

## Other concepts

1. `include` syntax - existing configuration files may be included by other files. There
   are no restrictions around a `proto` being defined in one file, then `reference`d in
   another file. This highlights one of the advantages of this syntax: the ability to
   define a generic set of templates in one file, which will be used to produce multiple,
   repeated concrete structs with different names and parameters in a different file.
2. [key-value reference](#key-value-references) - Much like bash, the syntax provides the ability
   to reference apreviously defined value by its key, and assign it to another key.
3. Appended keys - While a `proto` defines a templated `struct`, one can add additional keys
   to the resulting `struct` when the `proto` is referenced.
4. Fully qualified keys - One may define the configuration parameters using a combination
   of the tree-like structure found in json along with fully qualified key value pairs.
   These can not be mixed within the same file however.

While whitespace (except for following the `struct`, `proto`, `reference`, and `as` keywords)
is not significant it is convenient in order to better view the structure of the configuration parameters.

## Syntax details

### `struct` keyword

The `struct` keyword is used to define a structure of data, e.g.

```
struct foo {

  key1 = 0
  key2 = 1.4
  key3 = "string"

  struct bar {
    nested_key = 0
    baz = 3.14
  }

}
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
struct protos {

  proto foo {
    key1 = 0
    key2 = 1.4
    key3 = $KEY3

    struct bar {
      key_a = 0x1A2B
      key_b = $KEY_B
    }
  }

}
```

The above example defines a `proto` called "foo" with two concrete keys and a variable key `key3`. There is
also a nested struct (which also contains a contrete key and a variable key `key_b`).

This `proto` does not add anything to the configuration parameters as is. It must be `reference`d in order to
turn it into a concrete `struct` of data.

#### Variable

Keys within a `proto` (or any `struct` nested within a `proto`) may be assigned a "variable" which will be resolved later as explained below. A variable begins with a `$` and is followed by a variable name matching `[A-Z0-9_]+`. The variable name may optionally be enclosed in a set of curly braces (e.g. `{...}`).

Variables may also be used within strings (e.g. `"foo.$BAR.baz"`), within a [key-value reference](#key-value-references), in a list (e.g. `[0, ${ONE}, 2]`) or in an [expression](#mathematical-expressions) (e.g. `{{ 0.5 * ($MIN + ${MAX}) }}`)

### `reference` keyword

The `reference` keyword is used to turn a `proto` into a `struct`. Following on from the example above:

```
reference protos.foo as fuzz {
  $KEY3 = "apple"
  $KEY_B = 3.14159
  +extra_key = 0x7FF
}
```

The above example introduces the following concepts:

1. When referencing a `proto`, the fully qualified name of the proto must be used (i.e. `protos.foo` instead of `foo`).
2. The variables `$KEY3` and `$KEY_B` that were introduced in `protos.foo` and `protos.foo.bar` respectively are given values when the `proto` is `reference`d. These variables can be used in the following situations as explained [above](#variable)
    - Alone, as the value in a key/value pair
    - Within a string value (e.g. `key = "refer.to.$KEY3.thing"`)
    - Within a [key-value reference](#key-value-references) (e.g. `key = $(reference.$THIS.var)`)
    - Within a list
    - Within an expression
3. `+extra_key` is an appended key. The `proto` "protos.foo" does not contain this key, but the resulting `struct fuzz`
   will contain this additional key.

NOTE: There is a special variable `$PARENT_NAME` which can be used to assign the value of the containing parent key to a variable defined in a proto (e.g. we could write `$KEY3 = $PARENT_NAME` which would assign the value `"fuzz"` to `$KEY3`).

The above `reference` could be defined as follows using the `struct` syntax:

```
struct fuzz {
  key1 = 0
  key2 = 1.4
  key3 = "apple"
  extra_key = 0x7FF

  struct bar {
    key_a = 0x1A2B
    key_b = 3.14159
  }
}
```

### Key-value references

As mentioned above, existing values may be referenced by their key in order to define a different key/value pair. E.g.

```
struct foo {
  key1 = 0
  key2 = 1.4
}

struct bar {
  key1 = 1
  key2 = $(foo.key2)
}
```

In this case, the key `bar.key2` is given the value of `foo.key2`, or in this case `1.4`. This construct can be
used within the `struct`, `proto` or `reference` constructs. The syntax is `$(path.to.key)`. As mentioned in the
[`reference` section](#reference-keyword), a key-value reference may include variables internally (e.g. `$(path.to.$KEY)`.

### Supported value types

The following value types are supported:

1.  *Integers* (signed or unsigned)
2.  *Floating point* (standard or scientific notation)
3.  *Hexadecimal* (i.e. 0x[0-9a-fA-f]+, upper- or lower-case characers are supported). These resolve to an integer
4.  *Strings* - Any set of characters enclosed by double-quotes
5.  *Boolean* (`true` or `false`)
6.  *List/Array* - a bracket-enclosed, comma-separated list of same-typed values from the types listed above (i.e. one may define a list of strings or floating-point numbers, but not a mixed combination of the two).

### Mathematical expressions

The config syntax also supports the ability to express mathematical expressions using numbers and key-value references. When using key-value references within a mathematical expression, the value must evaluate to a numeric value (chained references are allowed as long as the end of the chain is numeric). Mathematical expressions are enclosed within a set of opening and closing curly braces (i.e. `{{` and `}}`). The keyword `pi` may also be used to represent the value of pi more precisely. E.g.

```
struct foo {
  key1 = 1.25
  key2 = -2
  val = {{ $(foo.key1) * $(foo.key2) }}
}

struct bar {
  key1 = 1e-2
  key2 = {{ $(foo.val) * $(bar.key1) ^ 0.5 + 10 }}
}
```

The following operators are supported:
*  `+` - plus (both binary and unary)
*  `-` - minus (both binary and unary)
*  `*` - multiply
*  `/` - divide
*  `^` or `**` - power
*  `pi` - the value of pi
*  `(` and `)` - Parentheses grouping operations

### Comments

The hash (`#`) symbol is used to add comments within a config file (think python syntax). Any characters after the `#` will be considered part of the comment until the end-of-line. This allows for trailing comments. While there is no multi-line comment character, multi-line comments can be achieved by placing a `#` at the beginning of each line.

```
# Comments can start at the beginning of a line

   # Leading spaces are also okay

struct foo {  # A trailing comment works like this
  key1 = 0
  # comments can also be embedded in a struct/proto/reference
  key2 = "a"  # We might leave a comment here to explain this variable
}

# A multi-line comment might look something
# like this, which is basically just a few
# normal comments in a row.
```

See [`config_example3.cfg`](examples/config_example3.cfg) and [`config_example5.cfg`](examples/config_example5.cfg) for additional examples of comment usage.

# Parsers

## C++
The C++ implementation uses the [`taocpp::pegtl`](https://github.com/taocpp/PEGTL) library to define the grammar.
`PEGTL` uses a templatized syntax to define the grammar (which can be found [here](cpp/config_grammar.h)).  This is used
for parsing the raw config file, along with a set of [actions](cpp/config_actions.h) which define how to act on the parse
output.  Once the raw config files are parsed, there is a second pass that does the following:
1. Finds all protos defined in the parsed output
2. Merges all nested structs into a single struct
3. Resolves all references (and their associated variables), turning references to `proto`s into `struct`s.
4. Unflattens any flat key/value pairs
5. Resolves key-value references

`PEGTL` also provides some additional functionality to analyze the defined grammar and to generate a parse-tree from a supplied configuration file.

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

 *  [`config_build`](src/config_build.cpp) - This application can be used to parse a config file and build the resulting config tree. Usage: `./src/config_reader ../example/config_example5.cfg`.
 *  [`config_reader_example`](src/config_reader_example.cpp) - This reads the [`config_example5.cfg`](examples/config_example5.cfg) configuration file and attempts to read a variety of variables from it. This uses a verbose mode, which generates a lot of debug printouts, tracing the parsing and construction of the config data.

## Python
The syntax is defined in python using PEG notation. The `pe` library is used to parse the PEG defined grammar, which
is used to parse the configuration files. There is then a post-processing step to fully resolve all protos, references,
variable references, etc. The end result is a nested dictionary of key value pairs. There are a variety of tests and
examples which demonstrate more of the capabilities of the language.

