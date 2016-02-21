Contributing
============
When contributing to Hourglass-Resurrection you agree that your contribution is licensed under GPLv2 and that it may become re-licensed to a different open-source license in the future.

To contribute, fork the project and make your changes under a branch name different than master, when you're happy, make a Pull Request.

Please format your pull request titles according to:  
[part]: [optional: functionality affected]: [short summary]

When contributing, please adhere to the coding standard.  
We know a lot of the code does not follow this standard, that's what happens when you fork a dead project, and then you're terrible at enforcing a standard for a while. This is _not_ an excuse to ignore the coding standard. If you have the time instead, please update existing code to follow the standard.

You may use any version of Visual Studio for the development locally, but the master repo must be buildable with Visual Studio 2013 for the time being.

Keep in mind that this project is aimed to work only with 32-bit programs and is dependant on 32-bit memory addressing to work. Therefore a lot of the coding standard is ignoring many problems that would arise compiling the code as a 64-bit program.

Hourglass-Resurrection Coding Standard
======================================

Applies to all code
-------------------
Try to maintain a 100-character line limit. If your code exceeds it with up to 5 characters, it's OK if the code looks better on one line. Otherwise, break it up.

Use spaces, and use tab-spacing 4. Always fine-tune alignment when lines are broken up.

Never use `using namespace [x];`, use `[x]::` prefixes instead.

Use `nullptr` over `NULL` wherever possible.

Always use C++ style casts.  
Do not use implicit casts. Use explicit casts for assignment, comparisons and function parameters.

Anything local to a file shall be declared in an anonymous namespace.

Make sure your code can, at least theoretically, compile with any Windows SDK from XP to most recent. Do try to check on MSDN for every used WinAPI function if it needs special includes etc. If there is a difference between header requirements between some versions of the Window SDKs, use proper `#ifdef` with Windows SDK defined values to handle it.

Includes
--------
MSVC C++ header include guard style (`#pragma once`).

Always include the necessary headers in the files that need them, even if a previously included file includes that header.

All header includes shall be at the top of the file.

Windows headers must be included first (Windows.h etc), then standard headers (vector etc), then project headers.

Use `<c[lib]>` instead of `<[lib].h>` when including from the C-libraries.

C-library vs C++ standard library
---------------------------------
Favor C-stdio over streams, as this is more compatible with the WinAPI and a bit faster execution-wise.

Unsafe functions
----------------
Microsoft-"declared unsafe" functions are a joke.  
Use C++ library functions as much as possible, when it's not possible, use the safest safe-variant from the C-library functions.  
Also do _not_ use functions that specify that the calling code has to clean up the stack.  
If an advisor says a WinAPI function is unsafe, use of that function is _not_ recommended, and a link showing the claim should be provided.

Constants
---------
**Naming:** UPPERCASE  
For numerical constants, use `enum` if the values of the numerical entities matter, otherwise use `enum class`. For other types of constants, use `static const`.

Types
-----
When specializing a type in order to enhance code clarity, use `using` over `#define` and `typedef`.
Example:
```
using ReturnValue = HRESULT;
using CallbackFunction = std::function<void(DWORD, DWORD, LONG)>;

template<class T>
using SpecialVector = std::vector<T>;
```

Local variables
---------------
**Naming:** lower_case_underscore  
**Global variable prefix:** `g_`  
**Member variable prefix:** `m_`  
**Static variable prefix:** `s_`  
Global/Member prefix has precedence of the static prefix, thus a static member is written as `ms_` and a static global is written as `gs_`.
Use meaningful names, do _not_ name the variable something like `my_awesome_var`.

Only one variable may be declared per line.

Always use WinAPI type names when they exist, otherwise use the standard types where applicable, last use built-in types, unless a built-in type exist that replaces the standard type.  
`auto` may only be used to express complex types in limited contexts such as iterators in loops, otherwise use the actual type.  
Exception: Do _not_ include `<cstdint>` etc to get typenames like `uint32_t`, use appropriate counterparts from the allowed ones instead.

Pointer binds to type unless there is a WinAPI typename for the pointer variant of the type, so use `LPWSTR` instead of `WCHAR*`, and `int* p` instead of `int *p`.  

Struct / Class / Enum
---------------------
**Naming:** CamelCase  
Use structs if all members will be exposed, otherwise classes with Get/Set function calls. No variable must be public in a class. When a class inherits from another class, the class it inherits from is always written on a new line.  
For example:
```
class AClass :
    AParentClass,
    AnotherParent
```
Get/Set methods are declared after the constructors and destructors, but before any other method. Get methods must always be `const`.  
Get/Set methods are named as follows:
```
[type] Get[VariableNameAsCamelCase]() const;
void Set[VariableNameAsCamelCase](const [type] value);
```

Functions and Methods
---------------------
**Naming:** CamelCase  
Parameter declaration rules:  
Pass-by-const-reference whenever the function takes an object it will not modify.  
Pass-by-pointer whenever the function will modify the data, and should modify it globally (i.e. extra return value).  
Pass-by-const-pointer whenever the function takes a pointer to a C-style array (such as a C-style string).  
Pass-by-value for anything else.

Templates
---------
When possible, declare them with the `template<>` part on its own line.

Brackets
--------
Always use brackets, always bracket on a new line. No single-line no-bracket statements please.  
Initializer lists of one level may have the brackets on the same line as the list, as long as this does not violate the line length limit.  
For example:  
`var array[] = { value, value, value };`  

Multi-level initializer lists may only have the innermost level brackets on the same line as the list.  
For example:  
```
var array[][][] = 
{
    {
        { value, value, value },
        { value, value, value }
    },
    {
        { value, value, value }
    }
};
```
Control Flow Statements
-----------------------
Always use a space between the keyword and the parenthesis.  
For example: `if (condition)`

Avoid empty loops where it is possible. If one must be declared, use this format:  
`while (condition) {}`

Prefer `while` or `for` over `do {} while` statements, but should you use a `do {} while` statement, the condition should be on the same line as the closing bracket.

When declaring loops, avoid `continue` as much as possible.

The only exception here is `sizeof`, which must _not_ use a space between keyword and parenthesis.

goto
----
`goto` may only be used if the code cannot be broken out into a function call in a nice manner.  
Such an example is in order to break out of a really complex loop etc.  
A `goto` must never leave the boundary of the function it's declared in.  
A `goto` must only leave scopes, not enter them.  
A `goto` must always move execution downwards in the source file.

Comments
--------
Always use `/* */`, and unless the comment has to exist mid-line, the comments shall have this format:
```
/*
 * A comment
 * and another line
 */
```
Sign comments that explain complicated code and/or steps that seem unnecessary/weird, and when signing, do so with your GitHub nickname. We know about git blame, but it's even easier to track things this way.  
Example of a final line signing:
```
 * -- Warepire
 */
```
EXE Special Cases
-----------------
Don't use `extern`, having to declare something `extern` in the executable means the code is just badly structured.

Maximum supported CommCtrl version for GUI code is version 4.70.

Don't use `#define` outside of changing header-configurations. Nothing should require it.

Wrap function-calls that need C-style function pointers into wrappers that take `std::function` type callbacks instead.

DLL Special Cases
-----------------
The use of `extern` is only OK if there is no other way to solve the problem in a nice manner.

The use of `#define` is only OK if it is used to heavily reduce code, especially duplication, in terms of using it for defining anamorphic values, and for macros that cannot be declared as inline functions.

When using C-style function pointers, the parameter type has to be declared like this:
```
using [function name] = [return type] ([calling convention] *)([argument types]);
```

Some code in the DLL may have to break coding standard, this may be OK, assuming proper reasoning can be supplied.

When it comes to coding in the DLL, we'd rather see a slightly broken coding standard regarding some things if it produces better structured code overall, just explain where necessary using signed comments.

### API hooks and tramps

**Hook/local re-creation prefix:** `My`  
**Trampoline prefix:** `Tramp`  
**Naming:** `[prefix][original name]`  
The API doc naming overrides all other coding style rules. So, if the API says the function is called `THIS_Function_naME`, then the hook is called `MyTHIS_Function_naME`.

This goes for everything! If the API calls a variable passed to a function `LPVOID lpvVarName__`, then the name of the variable in the hook variant is the same.

If the function being hooked is a class method, it will need a `this` pointer in the argument list, they are declared as `[pointer to class-type] thisptr`

If you need a return value variable for the return statement in a hooked function, it must always be named `rv` and be declared at the very top of the hooked function.

Pay close attention to W vs A suffix functions, the DLL handles both Unicode and ANSI encodings. When a template can be used to generate an anamorphic struct or COM variant where re-creation is necessary, the suffix must be N instead of W or A.
