#pragma once

#include "ConstexprAlgorithms.hpp"

namespace UnitGuard
{

// unit tag struct for distinguishing atomic units
struct AtomTag {};

/// A Power is "Base^Exp", e.g. Length^1, Time^-1, etc.
template< typename Base, int Exp >
struct Power
{
  using base_type = Base;
  static constexpr int exponent = Exp;

  static_assert( std::is_base_of< AtomTag, Base >::value, "Power: Base must inherit from AtomTag" );
};

// all Units we enforce compile-time constraints on are instantiations of "Unit", even non-composite units
template< typename... Powers >
struct Unit
{
  using powers = std::tuple< Powers... >;
  static_assert( std::conjunction< std::is_base_of< AtomTag, typename Powers::base_type >... >::value, "All Power::base_types... must be derived from AtomTag" );
};

template< typename U >
struct Negate;

template< typename... Ps >
struct Negate< Unit< Ps... > >
{
  using type = Unit< Power< typename Ps::base_type, -Ps::exponent >... >;
};

// Merge.hpp -----------------------------------------------------------------------------

template < typename UnsortedUnit, typename Head >
using PrependUnit = Prepend< Unit, UnsortedUnit, Head >;

template < typename UnitInstance, typename Pow >
struct MergeUnits;

// 1) Base case, the base unit was not already present in the `Unit`s pack
template < typename Pow >
struct MergeUnits< Unit< >, Pow >
{
  using type = Unit< Pow >;
};

// 2) When the first element shares the same base unit -> sum their exponents
template < typename Base, int E1, int E2, typename... Rest >
struct MergeUnits< Unit< Power< Base, E1 >, Rest... >, Power< Base, E2 > >
{
private:
  static constexpr int newExp = E1 + E2;
  // If the sum is zero, remove that base unit entirely
  using merged_tail = std::conditional_t<
    newExp == 0,
    Unit< Rest... >,
    Unit< Power< Base, newExp >, Rest... >
  >;
public:
  using type = merged_tail;
};

// 3) When the first element doesn't share the same base unit -> keep the head, recurse on the tail
template < typename FirstPow, typename Pow, typename... Rest >
struct MergeUnits< Unit< FirstPow, Rest... >, Pow >
{
private:
  using submerge = typename MergeUnits< Unit< Rest... >, Pow >::type;
public:
  using type = typename PrependUnit< submerge, FirstPow >::type;
};

// -----------------------------------------------------------------------------

/// AddPack<U1, U2>: Merge exponents by addition
template < typename U1, typename U2 >
struct AddPack;

// If the second Unit is empty, no changes
template < typename U1 >
struct AddPack< U1, Unit< > >
{
  using type = U1;
};

// Recursively add each Power from second to the first
template < typename P1, typename Pow, typename... Tail >
struct AddPack< P1, Unit< Pow, Tail... > >
{
private:
  using merged = typename MergeUnits< P1, Pow >::type;
public:
  using type = typename AddPack< merged, Unit< Tail... > >::type;
};

// -----------------------------------------------------------------------------

/// SubPack<U1, U2>: Merge exponents by subtracting the second from the first
template < typename U1, typename U2 >
struct SubPack;

// If the second Unit is empty, no changes
template < typename U1 >
struct SubPack< U1, Unit< > >
{
  using type = U1;
};

// Recursively subtract each Power from the first
template < typename U1, typename Pow, typename... Tail >
struct SubPack< U1, Unit< Pow, Tail... > >
{
private:
  // Subtracting Power<Base, E> is the same as adding Power<Base, -E>
  using NegativePow = Power< typename Pow::base_type, -Pow::exponent >;
  using merged = typename MergeUnits< U1, NegativePow >::type;
public:
  using type = typename SubPack< merged, Unit< Tail... > >::type;
};

// Comparison.hpp -----------------------------------------------------------------------------

// A simple trait that assigns each base type an integer “rank.”
template< typename Tag >
struct CanonicalOrder;

// The comparator uses CanonicalOrder< T >::value to compare.
struct CanonicalUnitComparitor
{
  template< typename P1, typename P2 >
  struct compare
  {
    static constexpr bool value = ( CanonicalOrder< typename P1::base_type >::value < CanonicalOrder< typename P2::base_type >::value );
  };
};

template < typename UnsortedUnit >
using CanonicalUnit = typename SortPack< Unit, CanonicalUnitComparitor, UnsortedUnit >::type;

/// are_same_units< U1, U2> : check if two Unit<...> have the same (Base,Exp) pairs
template < typename U1, typename U2 >
struct are_same_units;

template< typename... P1s, typename... P2s >
struct are_same_units< Unit< P1s... >, Unit< P2s... > >
{
private:
  // Sort both packs to canonical order
  using CanonicalU1 = typename CanonicalUnit< Unit< P1s... > >::type;
  using CanonicalU2 = typename CanonicalUnit<  Unit< P2s... > >::type;
public:
  static constexpr bool value = std::is_same< CanonicalU1, CanonicalU2 >::value;
};

// -----------------------------------------------------------------------------

/// Multiply<U1, U2> -> AddPack
template < typename U1, typename U2 >
struct Multiply
{
  using type = typename AddPack< U1, U2 >::type;
};

/// Divide<U1, U2> -> SubPack
template < typename U1, typename U2 >
struct Divide
{
  using type = typename SubPack< U1, U2 >::type;
};

/// Invert<U> -> Negate
template< typename U >
struct Invert
{
  using type = typename Negate< U >::type;
};

}