/*  This file is part of the Vc library. {{{
Copyright © 2013-2015 Matthias Kretz <kretz@kde.org>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the names of contributing organizations nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

}}}*/

#ifndef VC_COMMON_ALGORITHMS_H_
#define VC_COMMON_ALGORITHMS_H_

#include "macros.h"

namespace Vc_VERSIONED_NAMESPACE
{
/**
 * \ingroup Utilities
 *
 * \name Boolean Reductions
 */
//@{
/** \ingroup Utilities
 *  Returns whether all entries in the mask \p m are \c true.
 */
template<typename Mask> constexpr bool all_of(const Mask &m) { return m.isFull(); }
/** \ingroup Utilities
 *  Returns \p b
 */
constexpr bool all_of(bool b) { return b; }

/** \ingroup Utilities
 *  Returns whether at least one entry in the mask \p m is \c true.
 */
template<typename Mask> constexpr bool any_of(const Mask &m) { return m.isNotEmpty(); }
/** \ingroup Utilities
 *  Returns \p b
 */
constexpr bool any_of(bool b) { return b; }

/** \ingroup Utilities
 *  Returns whether all entries in the mask \p m are \c false.
 */
template<typename Mask> constexpr bool none_of(const Mask &m) { return m.isEmpty(); }
/** \ingroup Utilities
 *  Returns \p !b
 */
constexpr bool none_of(bool b) { return !b; }

/** \ingroup Utilities
 *  Returns whether at least one entry in \p m is \c true and at least one entry in \p m is \c
 *  false.
 */
template<typename Mask> constexpr bool some_of(const Mask &m) { return m.isMix(); }
/** \ingroup Utilities
 *  Returns \c false
 */
constexpr bool some_of(bool) { return false; }
//@}

#ifdef DOXYGEN
/**
 * \ingroup Utilities
 * \headerfile algorithms.h <Vc/Vc>
 *
 * Vc variant of the `std::for_each` algorithm.
 *
 * This algorithm calls \p f with one argument of type
 * `Vc::Vector<` *iterator value type* `, ` *unspecified* `>` as often as is needed to
 * iterate over the complete range from \p first to \p last.
 * It will try to use the best vector size (VectorAbi) to work on the largest chunks
 * possible.
 * To support aligned loads (and stores) and to support arbitrary range distances, the
 * algorithm may require the use of `Vc::VectorAbi` types that work on fewer elements in
 * parallel.
 *
 * The following example requires C++14 for generic lambdas. If you don't have generic
 * lambdas available you can use a "classic" functor type with a templated call operator
 * instead.
 *
 * \code
 * void scale(std::vector<double> &data, double factor) {
 *   Vc::simd_for_each(data.begin(), data.end(), [&](auto v) {
 *      v *= factor;
 *   });
 * }
 * \endcode
 */
template <class InputIt, class UnaryFunction>
UnaryFunction simd_for_each(InputIt first, InputIt last, UnaryFunction f);
#else
template <class InputIt, class UnaryFunction,
          class ValueType = typename std::iterator_traits<InputIt>::value_type>
inline enable_if<
    std::is_arithmetic<ValueType>::value &&
        Traits::is_functor_argument_immutable<UnaryFunction, Vector<ValueType>>::value,
    UnaryFunction>
simd_for_each(InputIt first, InputIt last, UnaryFunction f)
{
    typedef Vector<ValueType> V;
    typedef Scalar::Vector<ValueType> V1;
    for (; reinterpret_cast<std::uintptr_t>(std::addressof(*first)) &
                   (V::MemoryAlignment - 1) &&
               first != last;
         ++first) {
        f(V1(std::addressof(*first), Vc::Aligned));
    }
    const auto lastV = last - V::Size + 1;
    for (; first < lastV; first += V::Size) {
        f(V(std::addressof(*first), Vc::Aligned));
    }
    for (; first != last; ++first) {
        f(V1(std::addressof(*first), Vc::Aligned));
    }
    return std::move(f);
}

template <typename InputIt, typename UnaryFunction>
inline enable_if<
    std::is_arithmetic<typename std::iterator_traits<InputIt>::value_type>::value &&
        !Traits::is_functor_argument_immutable<
            UnaryFunction,
            Vector<typename std::iterator_traits<InputIt>::value_type>>::value,
    UnaryFunction>
simd_for_each(InputIt first, InputIt last, UnaryFunction f)
{
    typedef Vector<typename std::iterator_traits<InputIt>::value_type> V;
    typedef Scalar::Vector<typename std::iterator_traits<InputIt>::value_type> V1;
    for (; reinterpret_cast<std::uintptr_t>(std::addressof(*first)) &
                   (V::MemoryAlignment - 1) &&
               first != last;
         ++first) {
        V1 tmp(std::addressof(*first), Vc::Aligned);
        f(tmp);
        tmp.store(std::addressof(*first), Vc::Aligned);
    }
    const auto lastV = last - V::Size + 1;
    for (; first < lastV; first += V::Size) {
        V tmp(std::addressof(*first), Vc::Aligned);
        f(tmp);
        tmp.store(std::addressof(*first), Vc::Aligned);
    }
    for (; first != last; ++first) {
        V1 tmp(std::addressof(*first), Vc::Aligned);
        f(tmp);
        tmp.store(std::addressof(*first), Vc::Aligned);
    }
    return std::move(f);
}
#endif

template <typename InputIt, typename UnaryFunction>
inline enable_if<
    !std::is_arithmetic<typename std::iterator_traits<InputIt>::value_type>::value,
    UnaryFunction>
simd_for_each(InputIt first, InputIt last, UnaryFunction f)
{
    return std::for_each(first, last, std::move(f));
}

///////////////////////////////////////////////////////////////////////////////
template <typename InputIt, typename UnaryFunction>
inline enable_if<
    std::is_arithmetic<typename std::iterator_traits<InputIt>::value_type>::value &&
        Traits::is_functor_argument_immutable<
            UnaryFunction,
            Vector<typename std::iterator_traits<InputIt>::value_type>>::value,
    UnaryFunction>
simd_for_each_n(InputIt first, std::size_t count, UnaryFunction f)
{
    typename std::make_signed<size_t>::type len = count;
    typedef Vector<typename std::iterator_traits<InputIt>::value_type> V;
    typedef Scalar::Vector<typename std::iterator_traits<InputIt>::value_type> V1;
    for (; reinterpret_cast<std::uintptr_t>(std::addressof(*first)) &
               (V::MemoryAlignment - 1) &&
           len != 0;
         --len, ++first) {
        f(V1(std::addressof(*first), Vc::Aligned));
    }
    for (; len >= int(V::Size); len -= V::Size, first += V::Size) {
        f(V(std::addressof(*first), Vc::Aligned));
    }
    for (; len != 0; --len, ++first) {
        f(V1(std::addressof(*first), Vc::Aligned));
    }
    return std::move(f);
}

template <typename InputIt, typename UnaryFunction>
inline enable_if<
    std::is_arithmetic<typename std::iterator_traits<InputIt>::value_type>::value &&
        !Traits::is_functor_argument_immutable<
            UnaryFunction,
            Vector<typename std::iterator_traits<InputIt>::value_type>>::value,
    UnaryFunction>
simd_for_each_n(InputIt first, std::size_t count, UnaryFunction f)
{
    typename std::make_signed<size_t>::type len = count;
    typedef Vector<typename std::iterator_traits<InputIt>::value_type> V;
    typedef Scalar::Vector<typename std::iterator_traits<InputIt>::value_type> V1;
    for (; reinterpret_cast<std::uintptr_t>(std::addressof(*first)) &
               (V::MemoryAlignment - 1) &&
           len != 0;
         --len, ++first) {
        V1 tmp(std::addressof(*first), Vc::Aligned);
        f(tmp);
        tmp.store(std::addressof(*first), Vc::Aligned);
    }
    for (; len >= int(V::Size); len -= V::Size, first += V::Size) {
        V tmp(std::addressof(*first), Vc::Aligned);
        f(tmp);
        tmp.store(std::addressof(*first), Vc::Aligned);
    }
    for (; len != 0; --len, ++first) {
        V1 tmp(std::addressof(*first), Vc::Aligned);
        f(tmp);
        tmp.store(std::addressof(*first), Vc::Aligned);
    }
    return std::move(f);
}

#ifdef Vc_CXX17
#ifdef Vc_GCC
// GCC specific workaround because stdlibc++ doesn't have
// std::for_each_n implemented yet.
template <typename InputIt, typename UnaryFunction>
inline enable_if<!std::is_arithmetic<typename InputIt::value_type>::value, UnaryFunction>
simd_for_each_n(InputIt first, std::size_t count, UnaryFunction f)
{
    for (std::size_t i = 0; i < count; ++i, static_cast<void>(++first))
        std::apply(f, *first);
    return first;
}
#else
template <typename InputIt, typename UnaryFunction>
inline enable_if<
    !std::is_arithmetic<typename std::iterator_traits<InputIt>::value_type>::value,
    UnaryFunction>
simd_for_each_n(InputIt first, std::size_t count, UnaryFunction f)
{
    return std::for_each_n(first, count, std::move(f));
}
#endif
#endif

}  // namespace Vc

#endif // VC_COMMON_ALGORITHMS_H_
