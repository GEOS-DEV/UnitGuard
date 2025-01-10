
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

  static_assert( std::is_base_of< BaseUnit, Base >::value, "Power: Base must inherit from AtomTag" );
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

// Forward declaration for internal helper that merges a single Power<B,E> into a Unit< Power<B_i, E_i>... >
template < typename UnitT, typename Pow >
struct MergePow;

// MergePow when the Unit is empty -> just place the new Power
template < typename Pow >
struct MergePow< Unit< >, Pow >
{
  using type = Unit< Pow >;
};

// MergePow when the first element has the same base -> add exponents
template < typename Base, int E1, int E2, typename... Rest >
struct MergePow< Unit< Power< Base, E1 >, Rest... >, Power< Base, E2 > >
{
private:
  static constexpr int newExp = E1 + E2;
  // If the sum is zero, we drop that base unit from the list
  using merged_tail = std::conditional_t< newExp == 0, Unit< Rest... >, Unit< Power< Base, newExp >, Rest... > >;
public:
  using type = merged_tail;
};

// MergePow when the first element has a different base -> keep going
template < typename FirstPow, typename Pow, typename... Rest >
struct MergePow< Unit< FirstPow, Rest... >, Pow >
{
private:
  using submerge = typename MergePow< Unit< Rest... >, Pow >::type;
public:
  // Reinsert `FirstPow` at the front
  using type = Unit< FirstPow, typename submerge::Powers... >;
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
template < typename U1, typename Pow, typename... Tail >
struct AddPack< U1, Unit< Pow, Tail... > >
{
private:
  using merged = typename MergePow< U1, Pow >::type;
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
  using merged = typename MergePow< U1, NegativePow >::type;
public:
  using type = typename SubPack< merged, Unit< Tail... > >::type;
};

// Comparison.hpp -----------------------------------------------------------------------------



// Appends `Head` in front of `Unit<Powers...>`
template<typename UnitT, typename Head>
struct Prepend;

template<typename Head, typename... Powers>
struct Prepend<Unit<Powers...>, Head>
{
    using type = Unit<Head, Powers...>;
};


// A simple trait that assigns each base type an integer “rank.”
template< typename Tag >
struct CanonicalOrder;

template< typename P1, typename P2 >
struct LessThan
{
  // P1 and P2 are each PowUnit<SomeBase, Exp>.
  static constexpr bool value = ( CanonicalOrder< typename P1::base_type >::value < CanonicalOrder< typename P2::base_type >::value );
};

// A minimal compile-time insertion sort:
template< typename SortedList, typename Element >
struct InsertSorted;  // forward declaration

// Base case: Insert into an empty list
template< typename Element >
struct InsertSorted< Unit< >, Element >
{
  using type = Unit< Element >;
};

// Recursive: Compare with the first item of SortedList
template< typename First, typename... Rest, typename Element >
struct InsertSorted< Unit< First, Rest... >, Element >
{
  // Compare base orders: if Element < First, put Element in front
  static constexpr bool insertHere = LessThan< Element, First >::value;
  using TailSortedInsertion = typename InsertSorted< Unit< Rest... >, Element >::type;

  using type = std::conditional_t<
    insertHere,
    // If we insert here: [Element, First, Rest...]
    Unit< Element, First, Rest... >,
    // Keep First, then insert Element into the tail
    typename Prepend< TailSortedInsertion, First >::type
  >;
};

template < typename UnsortedList >
struct SortPowers;

// Empty list is already sorted
template < >
struct SortPowers< Unit< > >
{
  using type = Unit< >;
};

// If there's at least one power: sort the tail, then insert the head
template< typename First, typename... Rest >
struct SortPowers< Unit< First, Rest... > >
{
private:
  using SortedTail = typename SortPowers< Unit< Rest... > >::type;

public:
  using type = typename InsertSorted< SortedTail, First >::type;
};

/// are_same_units< U1, U2> : check if two Unit<...> have the same (Base,Exp) pairs
template < typename U1, typename U2 >
struct are_same_units;

template< typename... P1s, typename... P2s >
struct are_same_units< Unit< P1s... >, Unit< P2s... > >
{
private:
  // Sort both packs to canonical order
  using U1Sorted = typename SortPowers< U1 >::type;
  using U2Sorted = typename SortPowers< U2 >::type;
public:
  static constexpr bool value = std::is_same< U1Sorted, U2Sorted >::value;
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

/// Invert<U> -> NegatePack
template< typename U >
struct Invert
{
  using type = typename NegatePack< U >::type;
};

// -----------------------------------------------------------------------------

#define ENABLE_UNITGUARD 1;
#if ENABLE_UNITGUARD == 1
template < typename T, typename U >
class Quantity
{
public:
  T value;
  explicit Quantity( T v ) : value(v) {}

  // Convert to raw number
  operator T() const { return value; }

  // For printing
  operator std::string() const
  {
    return std::to_string(value);
  }

  template < typename _U >
  Quantity< T, U > & operator=( const Quantity< T, _U > & other )
  {
    static_assert( are_same_units< U, _U >::value, "Cannot assign incompatible units" );
    value = other.value;
    return *this;
  }

  template < typename _U >
  Quantity< T, U > & operator+=( const Quantity< T, _U > & other )
  {
    static_assert( are_same_units< U, _U >::value, "Cannot add different units" );
    value += other.value;
    return *this;
  }

  template < typename _U >
  Quantity< T, U > & operator-=( const Quantity< T, _U > & other )
  {
    static_assert( are_same_units< U, U >::value, "Cannot subtract different units" );
    value -= other.value;
    return *this;
  }

  Quantity< T, U > operator+( const Quantity & other ) const
  {
    return Quantity< T, U >( value + other.value );
  }

  Quantity< T, U > operator-( const Quantity & other ) const
  {
    return Quantity< T, U >( value - other.value );
  }

    // Multiply: results in new Unit with exponents added
  template< typename OU >
  auto operator*( const Quantity< T, OU > & other ) const
  {
    using ResultUnit = typename Multiply< U, OU >::type;
    return Quantity< T, ResultUnit >( value * other.value );
  }

  // Divide: results in new Unit with exponents subtracted
  template< typename OU >
  auto operator/( const Quantity< T, OU > & other ) const
  {
    using ResultUnit = typename Divide< U, OU >::type;
    return Quantity< T, ResultUnit >( value / other.value );
  }
};
#else
template < typename T, typename >
using Quantity = T;
#endif

// -----------------------------------------------------------------------------

#include <iostream>

template<typename T>
void printTypeName(const std::string & label)
{
  const std::type_info& ti = typeid(T);
  int status = -1;
  std::unique_ptr<char, void(*)(void*)> demangled_name(
      abi::__cxa_demangle(ti.name(), nullptr, nullptr, &status), std::free);
  std::cout << label << " => "
            << (status == 0 ? demangled_name.get() : ti.name())
            << std::endl;
}


// --------------------------------------------
// Fundamental tags for all atomic dimensions:
struct MassTag        : public AtomTag {};
struct LengthTag      : public AtomTag {};
struct TimeTag        : public AtomTag {};
struct CurrentTag     : public AtomTag {}; // e.g. electrice current
struct TemperatureTag : public AtomTag {};
struct AmmountTag     : public AtomTag {}; // e.g. moles
struct LuminanceTag   : public AtomTag {}; // e.g. candelas


// Establish canonical ordering
template < >
struct CanonicalOrder< MassTag >
{
  static constexpr int value = 0;
};

template < >
struct CanonicalOrder< LengthTag >
{
  static constexpr int value = 1;
};

template < >
struct CanonicalOrder< TimeTag >
{
  static constexpr int value = 2;
};

template < >
struct CanonicalOrder< CurrentTag >
{
  static constexpr int value = 3;
};


template < >
struct CanonicalOrder< TemperatureTag >
{
  static constexpr int value = 4;
};

template < >
struct CanonicalOrder< AmmountTag >
{
  static constexpr int value = 5;
};

template < >
struct CanonicalOrder< LuminanceTag >
{
  static constexpr int value = 6;
};

// --------------------------------------------
// Dimensionless (no base units at all):
using Dimensionless = Unit<>;

// --------------------------------------------
// Single-base (fundamental) units:
using MassDimension        = Unit<Power<MassTag,         1>>;
using LengthDimension      = Unit<Power<LengthTag,       1>>;
using TimeDimension        = Unit<Power<TimeTag,         1>>;
using CurrentDimension     = Unit<Power<CurrentTag,      1>>;
using TemperatureDimension = Unit<Power<TemperatureTag,  1>>;
using AmmountDimension     = Unit<Power<AmmountTag,      1>>;
using LuminanceDimension   = Unit<Power<LuminanceTag,    1>>;

// --------------------------------------------
// Derived units:

// Frequency = Time^-1
using FrequencyDimension = Unit< Power< TimeTag, -1 > >;

// Area = Length^2
using AreaDimension = Unit< Power< LengthTag, 2 > >;

// Volume = Length^3
using VolumeDimension = Unit< Power< LengthTag, 3 > >;

// Velocity = Length^1 * Time^-1
using VelocityDimension = Unit<
  Power< LengthTag, 1 >,
  Power< TimeTag,  -1 >
>;

// Acceleration = Length^1 * Time^-2
using AccelerationDimension = Unit<
  Power< LengthTag, 1 >,
  Power< TimeTag,  -2 >
>;

// Momentum = Mass^1 * Length^1 * Time^-1
using MomentumDimension = Unit<
  Power< MassTag,   1 >,
  Power< LengthTag, 1 >,
  Power< TimeTag,  -1 >
>;

// Force = Mass^1 * Length^1 * Time^-2
using ForceDimension = Unit<
  Power< MassTag,   1 >,
  Power< LengthTag, 1 >,
  Power< TimeTag,  -2 >
>;

// Pressure = Force / Area = Mass^1 * Length^-1 * Time^-2
using PressureDimension = Unit<
  Power< MassTag,   1 >,
  Power< LengthTag, -1 >,
  Power< TimeTag,  -2 >
>;

// Energy = Force * Distance = Mass^1 * Length^2 * Time^-2
using EnergyDimension = Unit<
  Power< MassTag,   1 >,
  Power< LengthTag, 2 >,
  Power< TimeTag,  -2 >
>;

// Power  = Energy / Time = Mass^1 * Length^2 * Time^-3
using PowerDimension = Unit<
  Power< MassTag,   1 >,
  Power< LengthTag, 2 >,
  Power< TimeTag,  -3 >
>;

// --------------------------------------------
// thermodynamic-derived

// Entropy = Energy / Temperature = Mass^1 * Length^2 * Time^-2 * Temperature^-1
using EntropyDimension = Unit<
  Power< MassTag,         1 >,
  Power< LengthTag,       2 >,
  Power< TimeTag,        -2 >,
  Power< TemperatureTag, -1 >
>;

// HeatCapacity = Energy / Temperature (identical to Entropy dimensionally)
using HeatCapacityDimension = EntropyDimension;

// ----------------------------------------------------------------------------

template < typename T > using Length      = Quantity< T, LengthDimension >;
template < typename T > using Mass        = Quantity< T, MassDimension >;
template < typename T > using Time        = Quantity< T, TimeDimension >;
template < typename T > using Temp        = Quantity< T, TemperatureDimension >;

// Derived:
template < typename T > using Frequency   = Quantity< T, FrequencyDimension >;
template < typename T > using Area        = Quantity< T, AreaDimension >;
template < typename T > using Volume      = Quantity< T, VolumeDimension >;
template < typename T > using Velocity    = Quantity< T, VelocityDimension >;
template < typename T > using Force       = Quantity< T, ForceDimension >;
template < typename T > using Pressure    = Quantity< T, PressureDimension >;
template < typename T > using Energy      = Quantity< T, EnergyDimension >;
template < typename T > using Entropy     = Quantity< T, EntropyDimension >;

// Optionally also define dimensionless (which is sometimes handy):
template < typename T > using Scalar      = Quantity< T, Dimensionless >;

}