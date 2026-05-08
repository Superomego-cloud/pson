The main feature introduced by PSON is variables and functions.

Variables are essentially containers for a redudant piece of data that the user
wishes to write more than once. 

Functions act the same way as class constructors, allowing you to define an object
with given attributes while specifying arguments and default values for said arguments. 

Variable functions and features are defined like so: 

```
var x = y, // defines the value of variable x to be y

func name(args){
    // expression of a JSON object
}
```
Additionally, to use a variable's value, you have to use angle brackets

```
<varname>                    // This is equivalent to the value contained within variable varname
<funcname(arg1, arg2, arg3)> // this is equivalent to calling the function funcname
                             // with the given args
```

using the <> operator is equivalent to a non-variable JSON value, so you can use it when assigning variables and when calling/defining functions

```
var varname = <var2name>
funcname(<varname>, <var2name>)
func funcname(){
    "attr1":<globalvar1>,
    <globalvar2>:"value",
    <globalvar3>:<globalvar4>
}
```