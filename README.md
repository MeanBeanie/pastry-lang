# pastry-lang

a simple programming language, non static types and no curly braces

### quirks

- all variables are global, once created can never be destroyed until the freeing of all variables at the end of execution
- for loops don't define variables, and can only do an increasing iteration (see todo)

### todo

1. implement read function(user input)
1. make for loops support step size(and negative step size)
1. implement while loops
1. implement if and else(maybe elif at some point)
1. add bash integration to make it actually useful
1. functions!!
1. temp variables(get deleted after scope)
    1. requires me to track scope

### fun other stuff todo

1. make macros/constants that let you add color/styling to print using ansi escape
1. command line input, (pass argv from main.c to the function after taking what i want)
1. include other files
    1. make a standard library style includable thing
    1. no headers, just include src(nothing can go wrong b/c if i don't look at ODR it might not hurt me)
1. confirmation to interpret if the file path doesn't have a .pastry in it
1. line tracking for errors(not relevant if you just don't make mistakes)

### examples

```
// fibonacci series
var x = 1
var y = 1

print x

var i = 0
var t = 0
for i 10
	t = y
	y = (x + y) // must be in parenthesis b/c my language is dumb
	x = t
	print x
end

print "X is " x " and y is " y
```
