# clox
clox is a bytecode interpreter for the lox programming language. Language details can be found in Robert Nystrom's excellently written book [Crafting Interpreters](http://craftinginterpreters.com).

## Overview
- Lox is a dynamically typed language with support for object-oriented and procedural programming paradigms.
- Lox was designed to teach the practical techniques used in implementing programming languages
- This implementation of lox is still ongoing and currently only supports variables, control flow, and functions
## Compiling

On `Ubuntu` with `gcc 9.2.1`:
```
$ git clone https://github.com/ibrahimsuleiman/clox.git
```
`cd` to the directory containing the source files and run `make` to build the executable `clox`. Then run
```
./clox
```
To run scripts, do
```
./clox [filename]
```
## Lox Basics

 ### *Hello, World*
```
> print("Hello, World");
Hello, World
```
### *Variables*

Variables can only begin with a letter or underscore. Digits are allowed anywhere but in the first position, and special characters are not allowed in variable names.
```
> var my_var = 42;
> my_var = "Forty two"; // re-assign
> var a;
> print(a);
nil
```
### *Expressions*
lox can be used as a calculator:
```
> (1 + 2) * (3 + 4) / 5;
 4.2
 ```
  ### *Control flow*
  All variables except `nil` are truthy in lox.
  
  *If-Else Statement*:
  ```
  > var b = true
  > if(!b) { 
          print("b is false");
     } else { 
          print("b is true");
     }
  b is true
  >
  ```
  *Infinite Loop*:
  ```
  > while(true) print("Hello");
  ```
  
  *For-Loop*:
  
  ```
  > for(var i = 0; i < 10; i = i + 1) { print(i); }
  ```
  ### *Functions*
  ```
  > fun sum(a, b) { return a + b; }
  > sum(3, 5);
  8
  ```
  *Recursion*
  
  ```
 > fun factorial(n) {
      if(n == 0) return 1;
      return n * factorial(n - 1);
 }
 > factorial(5)
 120
 ```
  *Getting Input*
  
  
```
> var name = input("What is your name?");
What is your name?
Ibrahim
> print("Hello, " + name);
Hello, Ibrahim
```
The built-in `input` function always returns a string. To get numbers, we call the `toNumber` function.
```
> var n = toNumber(input("Enter a number:"));
Enter a number:
5
> print(n);
5
> print(n * n);
25
```


  
  
