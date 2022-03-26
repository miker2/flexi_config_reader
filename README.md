# Flexi Config Reader

Flexi Config Reader is a configuration language that employs a template-reference syntax
in order to express repetattive & structured configuration parameters in a more concise
manner. Usage of these constructs is optional. At it's core, the syntax uses concepts
similar to those found in json & toml: nested structures of key/value pairs. One can draw
parallels between some of the features found in this syntax and those made available by
the jinja templating python library.

## Core concepts

The syntax has three core concepts:

1. `struct` - This is a concrete entry of data. A `struct` may contain other `struct`s as
   well as key/value pairs.
2. `proto` - The `proto` keyword is used to define a template, which can be used to create
   a `struct`. A `proto` may contain other `struct`s, as well as keys whos values are
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
   previously defined value by its key, and assign it to anoher key.
3. Appended keys - While a `proto` defines a templated `struct`, one can add additional keys
   to the `proto` when it is referenced.
4. Fully qualified keys - One may define the configuration parameters using a combination
   of the tree-like structure found in json along with fully qualified key value pairs.

While whitespace (except for following the `struct`, `proto`, `reference` and `end` keywords) is not significant
it is convenient in order to better view the structure of the configuration parameters.

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
3. `+extra_key` is an appended key. The `proto` "protos.foo" does not contain this key, but the result `struct fuzz`
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

As mentioned above, existing values may be reference by their key in order to define a different key/value pair. E.g.

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


## Python
The syntax is defined in python using PEG notation. The `pe` library is used to parse the PEG defined grammar, which
is used to parse the configuration files. There is then a post-processing step to fully resolve all protos, references,
variable references, etc. The end result is a nested dictionary of key value pairs. There are a variety of tests and
examples which demonstrate more of the capabilities of the language.

## C++
The C++ implementation uses the taocpp::pegtl library to define the grammar. So far this implementation lags behind
the python version and is currently only capable of generating a parse tree of the input configuration files.

# TODO

 - [ ] Add instructions on how to install dependencies
 - [ ] Add instructions on how to build the C++ code
 - [ ] Add instructions on how to run the tests
 - [ ] Add links to the tests and the examples within this document
