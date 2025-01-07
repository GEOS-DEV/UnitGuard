#include <iostream>
#include <type_traits>
#include <tuple>
#include <cxxabi.h>
#include <typeinfo>
#include <memory>
#include <cstdlib>
#include <string>

// Base unit struct for distinguishing different units
struct BaseUnit {};

template<typename... Units>
struct Unit {
    // Static assert to check all Units are derived from BaseUnit
    static_assert(std::conjunction<std::is_base_of<BaseUnit, Units>...>::value,
                  "All units in Unit must be subclasses of BaseUnit");
};

// Specific physical quantity tags
struct Length : BaseUnit {};
struct Mass : BaseUnit {};
struct Time : BaseUnit {};

// Template to represent inverse units
template<typename U>
struct InverseUnit : BaseUnit 
{
    static_assert(std::is_base_of<BaseUnit, U>::value, "U must be a subclass of BaseUnit");
};

template<typename U>
struct Invert;

// Example of a compound unit: Length*Time^-1
using VelocityUnit = Unit<Length, InverseUnit<Time>>;

template<typename U1, typename U2>
struct CanMultiply : std::true_type {};

template<typename U1, typename U2>
struct CanDivide : std::true_type {};

/// Unit Simplification Logic
template <typename>
struct SimplifyUnit;

template<>
struct SimplifyUnit<Unit<>> {
    using type = Unit<>;
};


// Type trait to detect inverse units
template<typename U>
struct IsInverseUnit : std::false_type {};

template<typename U>
struct IsInverseUnit<InverseUnit<U>> : std::true_type {};




template<typename List, typename U>
struct AppendUnit;

template<typename... Units, typename U>
struct AppendUnit<Unit<Units...>, U> {
    using type = Unit<Units..., U>;
};



// Base template for a single unit, enabled only when U is not an InverseUnit
template<typename U>
struct Invert {
    using type = InverseUnit<U>;
};

// Specialization for an InverseUnit to strip one layer of inversion
template<typename U>
struct Invert<InverseUnit<U>> {
    using type = U;  // Simplifies the double inversion
};

// Helper to invert a unit pack
template<typename... Units>
struct InvertUnitPack;

template<typename U, typename... Us>
struct InvertUnitPack<U, Us...> {
    using type = typename AppendUnit<typename InvertUnitPack<Us...>::type, typename Invert<U>::type>::type;
};

template<>
struct InvertUnitPack<> {
    using type = Unit<>;
};


template<typename List, typename U>
struct RemoveUnit;

// Base case: When the list is empty
template<typename U>
struct RemoveUnit<Unit<>, U> {
    using type = Unit<>;
};

// Recursive case: when the first type matches the type to remove
template<typename First, typename... Rest>
struct RemoveUnit<Unit<First, Rest...>, First> {
    using type = Unit<Rest...>;  // Remove First and return the rest
};

// Recursive case: when the first type does not match
template<typename First, typename... Rest, typename U>
struct RemoveUnit<Unit<First, Rest...>, U> {
    using type = typename AppendUnit<typename RemoveUnit<Unit<Rest...>, U>::type, First>::type;
};

static_assert(std::is_same<typename RemoveUnit<Unit<Time>, Time>::type, Unit<>>::value, "Unit<Time> should be removed, leaving an empty list.");

template<typename List, typename T>
struct CountType;

template<typename T>
struct CountType<Unit<>, T> : std::integral_constant<int, 0> {};

// Recursive case: Increment count if First is the same type as T, then continue checking the rest.
template<typename First, typename... Rest, typename T>
struct CountType<Unit<First, Rest...>, T> 
    : std::integral_constant<int, std::is_same<Unit<First>, Unit<T>>::value + CountType<Unit<Rest...>, T>::value> {};



template<typename Unit1, typename Unit2>
struct Multiply;

template<typename... Units1, typename... Units2>
struct Multiply<Unit<Units1...>, Unit<Units2...>> {
    using type = typename SimplifyUnit<Unit<Units1..., Units2...>>::type;
};



template<typename Unit1, typename Unit2>
struct Divide;

// Modified Divide to use InvertUnitPack for the second unit pack
template<typename... Units1, typename... Units2>
struct Divide<Unit<Units1...>, Unit<Units2...>> {
    using InvertedUnits2 = typename InvertUnitPack<Units2...>::type;
    using type = typename Multiply<Unit<Units1...>, InvertedUnits2>::type;
};



template< typename List >
struct CancelUnits;

template<>
struct CancelUnits<Unit<>> {
    using type = Unit<>;
};

template< typename U >
struct CancelUnits<Unit<U>> {
    using type = Unit<U>;
};

template<typename First, typename... Rest>
struct CancelUnits<Unit<First, Rest...>> {
    using RestSimplified = typename CancelUnits<Unit<Rest...>>::type;
    // static_assert( std::is_same< RestSimplified, Unit<Time> >::value );
    
    // Determine if the inverse of First exists in the simplified rest
    static constexpr bool hasInverse = CountType<RestSimplified, typename Invert<First>::type>::value > 0;
    // static_assert( CountType<RestSimplified, typename Invert<First>::type>::value == 1 );
    // static_assert( hasInverse );

    // static_assert( std::is_same< typename Invert<First>::type, Time >::value );

    // Type used to remove the inverse of First if found
    using RestWithoutInverse = typename std::conditional<
        hasInverse,
        typename RemoveUnit<RestSimplified, typename Invert<First>::type>::type,
        RestSimplified
    >::type;
    // static_assert( std::is_same< typename RemoveUnit<RestSimplified, typename Invert<First>::type>::type, Unit<Time> >::value );
    // static_assert( std::is_same< RestWithoutInverse, Unit<> >::value );

    // Recurse on the potentially modified list after modification (removal of First and its inverse)
    using type = typename std::conditional<
        hasInverse,
        typename CancelUnits<RestWithoutInverse>::type,  // Continue simplification without First and its inverse
        typename AppendUnit<RestWithoutInverse, First>::type  // No inverse found, add First back to the list and continue
    >::type;
};

template<typename... Units>
struct SimplifyUnit<Unit<Units...>> {
    using type = typename CancelUnits<Unit<Units...>>::type;
};


template<typename List1, typename List2>
struct EquivalentUnits;

template<typename... Types1, typename... Types2>
struct EquivalentUnits<Unit<Types1...>, Unit<Types2...>> {
    static constexpr bool compare_each() {
        return (... && (CountType<Unit<Types1...>, Types2>::value == CountType<Unit<Types2...>, Types1>::value));
    }

    static constexpr bool value = compare_each();
};


// template<typename T, typename U>
// class Quantity {
// public:
//     T value;

//     explicit Quantity(T v) : value(v) {}

//     operator T() const {
//         return value;
//     }

//     operator std::string() const {
//         return std::to_string(value);  // Removed demangle for simplification
//     }

//     // Ensure units are the same for addition and subtraction
//     Quantity& operator+=(const Quantity& other) {
//         static_assert(std::is_same<U, U>::value, "Cannot add different units");
//         value += other.value;
//         return *this;
//     }

//     Quantity& operator-=(const Quantity& other) {
//         static_assert(std::is_same<U, U>::value, "Cannot subtract different units");
//         value -= other.value;
//         return *this;
//     }

//     Quantity operator+(const Quantity& other) const {
//         return Quantity(value + other.value);
//     }

//     Quantity operator-(const Quantity& other) const {
//         return Quantity(value - other.value);
//     }

//     // Multiplication (results in compound unit)
//     template<typename OU>
//     auto operator*(const Quantity<T, OU>& other) const {
//         static_assert(CanMultiply<U, OU>::value, "Cannot multiply these units");
//         using ResultUnit = typename SimplifyUnit< typename Multiply< U, OU >::type >::type;
//         return Quantity<T, ResultUnit>(value * other.value);
//     }

//     // Division (results in unit ratio)
//     template<typename OU>
//     auto operator/(const Quantity<T, OU>& other) const {
//         static_assert(CanDivide<U, OU>::value, "Cannot divide these units");
//         using ResultUnit = typename SimplifyUnit< typename Divide< U, OU >::type >::type;
//         return Quantity<T, Unit<>>(value / other.value);
//     }
// };

// Quantity< double, Length > operator "" _L( long double length ) { return Quantity< double, Length >( length ); }
// Quantity< double, Length > operator "" _L( unsigned long long length ) { return Quantity< double, Length >( length ); }



template<typename T>
void printTypeName() {
    const std::type_info& ti = typeid(T);
    int status = -1;
    std::unique_ptr<char, void(*)(void*)> demangled_name(
        abi::__cxa_demangle(ti.name(), nullptr, nullptr, &status), std::free);
    std::cout << "The type is: " << (status == 0 ? demangled_name.get() : ti.name()) << std::endl;
}
