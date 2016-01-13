/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

 #pragma once

/*
 * ------------------------------------------------------------------------------------------------
 *                                              MACROS
 * ------------------------------------------------------------------------------------------------
 */

/*
 * Assert macro for fatal states in the DLL.
 * When Hourglass captures the exception it prints the call stack which in combination with the
 * rest of the log gives us an idea how the bad state was reached.
 * This can't be a function call as that may in some cases alter the call stack too much.
 */
 #define DLL_ASSERT(expression) \
    do \
    { \
        if ((expression) == false) \
        { \
            _asm \
            { \
                int 3\
            }; \
        } \
    } while (false)


/*
 * ------------------------------------------------------------------------------------------------
 *                                              TYPES
 * ------------------------------------------------------------------------------------------------
 */

/*
 * Lazy intitialization type.
 * Every global variable of non-fundamental type must be declared as one of these,
 * this ensures the variable is not instanciated before it's used.
 * It's important to avoid too early instanciation of non-fundamental types as they have
 * constructors which will do undesirable things.
 */
template<class T>
class LazyType
{
public:
    LazyType() {}
    LazyType(LazyType&) = delete;
    LazyType<T>& operator= (LazyType&) = delete;
    ~LazyType() {}

    T& operator() () const
    {
        static T the_variable;
        return the_variable;
    }
};
