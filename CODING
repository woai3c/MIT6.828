JOS CODING STANDARDS

It's easier on everyone if all authors working on a shared
code base are consistent in the way they write their programs.
We have the following conventions in our code:

* No space after the name of a function in a call
  For example, printf("hello") not printf ("hello").

* One space after keywords "if", "for", "while", "switch".
  For example, if (x) not if(x).

* Space before braces.
  For example, if (x) { not if (x){.

* Function names are all lower-case separated by underscores.

* Beginning-of-line indentation via tabs, not spaces.

* Preprocessor macros are always UPPERCASE.
  There are a few grandfathered exceptions: assert, panic,
  static_assert, offsetof.

* Pointer types have spaces: (uint16_t *) not (uint16_t*).

* Multi-word names are lower_case_with_underscores.

* Comments in imported code are usually C /* ... */ comments.
  Comments in new code are C++ style //.

* In a function definition, the function name starts a new line.
  Then you can grep -n '^foo' */*.c to find the definition of foo.

* Functions that take no arguments are declared f(void) not f().

The included .dir-locals.el file will automatically set up the basic
indentation style in Emacs.
