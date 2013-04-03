/**
 * @file Delegate.h
 *
 * @brief Implementation of unicast delegates for C++
 *
 * @author Ilya Skriblovsky
 * @date 2009
 *
 * This class was written after reading of
 *     http://www.codeproject.com/KB/cpp/FastDelegate.aspx
 *
 * Use it as follows:
 *
 * @code
 *   // Delegate with one char* parameter and returning int
 *   LightCore::Delegate1<char*, int> calcHash; 
 *
 *   // bind it to member function:
 *   calcHash = LightCore::Delegate(&hasher, &Hasher::calcCRC32);
 *
 *   // or to static function:
 *   calcHash = LightCore::Delegate(&Hasher::staticCalcCRC32);
 *
 *   // call it (after checking for non-zero)
 *   if (calcHash)
 *       hash = calcHash(data);
 * @endcode
 *
 * @warning Delegates are implemented with performance in mind, so them, for
 * example, aren't checked for zero when calling.
 *
 * There are several DelegateX classes where X is number of parameters of the
 * delegate. If you need delegate with no parameters, use Delegate0<>, if you
 * need delegate with 1 parameter, use Delegate1<type> and so on. You can
 * easily add more DelegateX classes in this file using Delegate1 as template.
 *
 * Known limitations:
 *   - Implementation is compiler-specific and tested only on GCC and
 *     Microsoft VC++.
 *   - If you create delegate like this:
 *         Delegate((Base*)derived, &Base::virtualFunction)
 *     then delegate will call Base::virtualFunction, not 
 *     Derived::virtualFunction. This is due to semantics of member pointers
 *   - Probably wont work with multiple inheritance
 *   - Const member functions are not (yet) supported
 *
 */

#ifndef __DELEGATE_H
#define __DELEGATE_H

#ifndef DEBUG
#include <stdio.h>
#endif

//namespace LightCore
//{

/// We use pointers to this undefined class for internal storage
/**
 * There is The Main Quirk: we cast any object pointer to the pointer to object
 * of __UndefinedClass and any member pointer to the pointer to member of
 * __UndefinedClass. When we later call it, original member pointer will be
 * called. This violates C++ standard, but works with every compiler.
 *
 * Microsoft compiler uses more complex member pointers for classes with
 * multiple inheritance. So we must explicitly declare __UndefinedClass
 * having single inheritance
 */
#ifndef __GNUC__
class __single_inheritance __UndefinedClass;
#else
class __UndefinedClass;
#endif

/// The internal core of delegate object
/**
 * This class receives object and method as constructor parameters and casts
 * them to __UndefinedClass* and __UndefinedClass' member pointer.
 *
 * We don't know number and types of delegate's parameters, so we need to pass
 * method type as template parameter. Delegate classes will pass different
 * types of __UndefinedClass' member pointers as Member parameter.
 *
 * There is an important trick when creating delegate pointing to static
 * function. In this case we store pointer to static function as instance
 * pointer * of the delegate, and pointer to DelegateX::staticInvoker method
 * as method pointer. When delegate is called, it jumps to
 * DelegateX::staticInvoker. This method casts \c this pointer to pointer to
 * static function with specified number of params and return type, and then
 * calls it. Dark magic, but it works.
 */
template<class Method>
class DelegateCore
{
    private:
        /// Instance pointer, casted to __UndefinedClass*
        __UndefinedClass *m_obj;
        /// Member pointer
        Method m_method;

    public:
        /// Default constructor
        /**
         * Fills the instance and method pointers with NULLs
         */
        DelegateCore()
        {
            m_obj = 0;
            m_method = 0;
        }

        /// Copy constructor
        DelegateCore(const DelegateCore<Method>& other)
        {
            m_obj = other.m_obj;
            m_method = other.m_method;
        }

        /// Creates DelegateCore with specified instance and method pointers
        template<class T, class M> DelegateCore(T *obj, M method)
        {
            m_obj = reinterpret_cast<__UndefinedClass*>(obj);
            m_method = reinterpret_cast<Method>(method);
        }

        /// Returns stored instance pointer
        inline __UndefinedClass* obj() const { return m_obj; }
        /// Returns stored method pointer
        inline Method method() const { return m_method; }
};

// MSVC version 6 ant earlier doesn't allow template parameter to have default void
#if defined(_MSC_VER) && (_MSC_VER <1300)
#define LIGHTCORE_DELEGATE_DEFAULTVOID_HACK
#endif

#ifdef LIGHTCORE_DELEGATE_DEFAULTVOID_HACK
typedef const void* DefaultVoid;
#else
typedef void DefaultVoid;
#endif

template <class T>
struct DefaultVoidToVoid { typedef T type; };
template <>
struct DefaultVoidToVoid<DefaultVoid> { typedef void type; };

template <class T>
struct VoidToDefaultVoid { typedef T type; };
template <>
struct VoidToDefaultVoid<void> { typedef DefaultVoid type; };

#ifdef LIGHTCORE_DELEGATE_DEFAULTVOID_HACK
    #define LIGHTCORE_DELEGATE_RETTYPE   VoidToDefaultVoid<RetType>::type
#else
    #define LIGHTCORE_DELEGATE_RETTYPE   RetType
#endif


/// Delegate without parameters
/**
 * @param RetType Delegate's return type. Defaults to void.
 *
 * @warning Defaulting RetType to void may not work with VC6 or earlier
 */
template<class RetType = DefaultVoid>
class Delegate0
{
    public:
        typedef typename DefaultVoidToVoid<RetType>::type ReturnType;

        /// We cast all method pointers to this type
        typedef RetType (__UndefinedClass::*Method)();
        /// And this is for static functions
        typedef RetType (*StaticFunc)();

        typedef ReturnType (*VoidStaticFunc)();

    private:
        DelegateCore<Method> core; /// Core object that stores the pointers

        /// Invoker for static functions
        /** Trick is used for static funcions: when binding to static function
         * we actually bind delegate to staticInvoker and pass original
         * function's address as this. See comments on DelegateCore.
         */
        RetType staticInvoker()
        { return ((StaticFunc)(this))(); }

    public:
        /// Default constructor
        /**
         * Initializes delegate with zero pointers
         */
        Delegate0() { } // Core object will initialize itself with 0

        /// Copy constructor
        Delegate0(const Delegate0<RetType>& other)
        { core = other.core; }

        /// Create delegate pointing to specified object and method
        template<class T>
            Delegate0(T *obj, ReturnType (T::*method)())
                : core(obj, method)
            { }

        /// Create delegate pointing to static function
        Delegate0(VoidStaticFunc func)
            : core((Delegate0*)func, &Delegate0::staticInvoker)
        { }

        /// Calls the delegate
        /**
         * @warning Delegate is not checked for zero. Use it as follows:
         * @code if (delegate) delegate.call() @endcode
         */
        inline RetType call() const
        { return (core.obj()->*core.method())(); }

        /// Calls the delegate
        /**
         * @warning Delegate is not checked for zero. Use it as follows:
         * @code if (delegate) delegate() @endcode
         */
        inline RetType operator ()() const
        { return call(); }

        /// Checks whether the delegate is uninitialized
        inline bool empty() const
        { return core.obj() == 0 || core.method() == 0; }

        /// Checks whether the delegate is initialized
        inline bool operator ! () const
        { return empty(); }

        /// Checks whether the delegate is initialized
        inline operator bool () const
        { return !empty(); }
};

/// Create delegate pointing to specified method or static function
/**
 * No need to specify template arguments when creating Delegate with this 
 * function.
 *
 * You may use it simply like this:
 * @code
 * Delegate0<> d = Delegate(&object, &Object::method);
 * Delegate0<> d = Delegate(&staticFunction);
 * @endcode
 */
template<class T, class X, class RetType>
Delegate0<LIGHTCORE_DELEGATE_RETTYPE> Delegate(T *obj, RetType (X::*method)())
{ return Delegate0<LIGHTCORE_DELEGATE_RETTYPE>(obj, method); }

/// Create delegate pointing to static function
/**
 * No need to specify template arguments when creating Delegate with this 
 * function.
 *
 * You may use it simply like this:
 * @code
 * @endcode
 */
template<class RetType>
Delegate0<LIGHTCORE_DELEGATE_RETTYPE> Delegate(RetType (*staticFunc)())
{ return Delegate0<LIGHTCORE_DELEGATE_RETTYPE>(staticFunc); }


/// Delegate with one parameter
/**
 * All methods are same as in Delegate0, so refer documentation for that class
 *
 * @param P1    Type of delegate parameter
 * @param RetType   Type of returning value
 */
template<class P1, class RetType = DefaultVoid>
class Delegate1
{
    private:
        typedef typename DefaultVoidToVoid<RetType>::type ReturnType;

        typedef RetType (__UndefinedClass::*Method)(P1);
        typedef RetType (*StaticFunc)(P1);
        typedef ReturnType (*VoidStaticFunc)(P1);
        DelegateCore<Method> core;
        RetType staticInvoker(P1 p1)
        { return ((StaticFunc)(this))(p1); }

    public:
        Delegate1() { }

        Delegate1(const Delegate1<P1, RetType>& other)
        { core = other.core; }

        template<class T>
            Delegate1(T *obj, ReturnType (T::*method)(P1))
                : core(obj, method)
            { }

        Delegate1(VoidStaticFunc func)
            : core((Delegate1*)func, &Delegate1::staticInvoker)
        { }

        inline RetType call(P1 p1) const
        { return (core.obj()->*core.method())(p1); }

//#ifdef DEBUG
//        inline void debug() const
//        {
//            printf("Delegate1:\n\tthis=0x%x\n\tobj=0x%x\n\tmethod=0x%x\n", this, core.obj(), core.method());
//        }
//#endif

        inline RetType operator () (P1 p1) const
        { return call(p1); }

        inline bool empty() const
        { return core.obj() == 0 || core.method() == 0; }

        inline bool operator ! () const
        { return empty(); }

        inline operator bool () const
        { return !empty(); }
};


template<class T, class X, class P1, class RetType>
Delegate1<P1, LIGHTCORE_DELEGATE_RETTYPE> Delegate(T *obj, RetType (X::*method)(P1))
{ return Delegate1<P1, LIGHTCORE_DELEGATE_RETTYPE>(obj, method); }

template<class P1, class RetType>
Delegate1<P1, LIGHTCORE_DELEGATE_RETTYPE> Delegate(RetType (*staticFunc)(P1))
{ return Delegate1<P1, LIGHTCORE_DELEGATE_RETTYPE>(staticFunc); }



/// Delegate with two parameters
/**
 * All methods are same as in Delegate0, so refer documentation for that class
 *
 * @param P1        Type of first delegate parameter
 * @param P2        Type of second delegate parameter
 * @param RetType   Type of returning value
 */
template<class P1, class P2, class RetType = DefaultVoid>
class Delegate2
{
    private:
        typedef typename DefaultVoidToVoid<RetType>::type ReturnType;

        typedef RetType (__UndefinedClass::*Method)(P1, P2);
        typedef RetType (*StaticFunc)(P1, P2);
        typedef ReturnType (*VoidStaticFunc)(P1, P2);
        DelegateCore<Method> core;
        RetType staticInvoker(P1 p1, P2 p2)
        { return ((StaticFunc)(this))(p1, p2); }

    public:
        Delegate2() { }

        Delegate2(const Delegate2<P1, P2, RetType>& other)
        { core = other.core; }

        template<class T>
            Delegate2(T *obj, ReturnType (T::*method)(P1, P2))
                : core(obj, method)
            { }

        Delegate2(VoidStaticFunc func)
            : core((Delegate2*)func, &Delegate2::staticInvoker)
        { }

        inline RetType call(P1 p1, P2 p2) const
        { return (core.obj()->*core.method())(p1, p2); }

        inline RetType operator () (P1 p1, P2 p2) const
        { return call(p1, p2); }

        inline bool empty() const
        { return core.obj() == 0 || core.method() == 0; }

        inline bool operator ! () const
        { return empty(); }

        inline operator bool () const
        { return !empty(); }
};

template <class T, class X, class P1, class P2, class RetType>
Delegate2<P1, P2, LIGHTCORE_DELEGATE_RETTYPE> Delegate(T *obj, RetType (X::*method)(P1, P2))
{ return Delegate2<P1, P2, LIGHTCORE_DELEGATE_RETTYPE>(obj, method); }

template <class P1, class P2, class RetType>
Delegate2<P1, P2, LIGHTCORE_DELEGATE_RETTYPE> Delegate(RetType (*staticFunc)(P1, P2))
{ return Delegate2<P1, P2, LIGHTCORE_DELEGATE_RETTYPE>(staticFunc); }



/// Delegate with three parameters
/**
 * All methods are same as in Delegate0, so refer documentation for that class
 *
 * @param P1        Type of first delegate parameter
 * @param P2        Type of second delegate parameter
 * @param P3        Type of third delegate parameter
 * @param RetType   Type of returning value
 */
template<class P1, class P2, class P3, class RetType = DefaultVoid>
class Delegate3
{
    private:
        typedef typename DefaultVoidToVoid<RetType>::type ReturnType;

        typedef RetType (__UndefinedClass::*Method)(P1, P2, P3);
        typedef RetType (*StaticFunc)(P1, P2, P3);
        typedef ReturnType (*VoidStaticFunc)(P1, P2, P3);
        DelegateCore<Method> core;
        RetType staticInvoker(P1 p1, P2 p2, P3 p3)
        { return ((StaticFunc)(this))(p1, p2, p3); }

    public:
        Delegate3() { }

        Delegate3(const Delegate3<P1, P2, P3, RetType>& other)
        { core = other.core; }

        template<class T>
            Delegate3(T *obj, ReturnType (T::*method)(P1, P2, P3))
                : core(obj, method)
            { }

        Delegate3(VoidStaticFunc func)
            : core((Delegate3*)func, &Delegate3::staticInvoker)
        { }

        inline RetType call(P1 p1, P2 p2, P3 p3) const
        { return (core.obj()->*core.method())(p1, p2, p3); }

        inline RetType operator () (P1 p1, P2 p2, P3 p3) const
        { return call(p1, p2, p3); }

        inline bool empty() const
        { return core.obj() == 0 || core.method() == 0; }

        inline bool operator ! () const
        { return empty(); }

        inline operator bool () const
        { return ! empty(); }
};

template <class T, class X, class P1, class P2, class P3, class RetType>
Delegate3<P1, P2, P3, LIGHTCORE_DELEGATE_RETTYPE> Delegate(T *obj, RetType (X::*method)(P1, P2, P3))
{ return Delegate3<P1, P2, P3, LIGHTCORE_DELEGATE_RETTYPE>(obj, method); }

template <class P1, class P2, class P3, class RetType>
Delegate3<P1, P2, P3, LIGHTCORE_DELEGATE_RETTYPE> Delegate(RetType (*staticFunc)(P1, P2, P3))
{ return Delegate3<P1, P2, P3, LIGHTCORE_DELEGATE_RETTYPE>(staticFunc); }


//}


#endif
