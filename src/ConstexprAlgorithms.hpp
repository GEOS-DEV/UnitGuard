#pragma once

namespace UnitGuard
{

// Prepend -----------------------------------------------------------------------------

// Forward declaration
template< template< typename... > class VariadicTemplate, typename VariadicInstance, typename Head >
struct Prepend;

// Partial specialization for the instance = `VariadicTemplate<...>`
template< template <typename... > class VariadicTemplate, typename Head, typename... Ts >
struct Prepend< VariadicTemplate, VariadicTemplate< Ts... >, Head >
{
  using type = VariadicTemplate< Head, Ts... >;
};

// InsertSorted -----------------------------------------------------------------------------

// Forward declaration
template<
  template<typename...> class VariadicTemplate,
  typename Comparator,
  typename SortedList,
  typename Element
>
struct InsertSorted;

// Base case: SortedList is empty
template< template<typename...> class VariadicTemplate, typename Comparator, typename Element >
struct InsertSorted< VariadicTemplate, Comparator, VariadicTemplate<>, Element >
{
  using type = VariadicTemplate< Element >;
};

// Recursive case
template<
  template<typename...> class VariadicTemplate,
  typename Comparator,
  typename First,
  typename... Rest,
  typename Element
>
struct InsertSorted< VariadicTemplate, Comparator, VariadicTemplate< First, Rest... >, Element >
{
private:
  // Compare using our user-supplied comparator
  static constexpr bool insertHere = Comparator::template compare< Element, First >::value;

  // If we don't insert here, we recursively insert into the tail
  using TailInsertion = typename InsertSorted< VariadicTemplate, Comparator, VariadicTemplate< Rest... >, Element >::type;

public:
  using type = std::conditional_t<
    insertHere,
    // Insert Element before First
    VariadicTemplate< Element, First, Rest... >,
    // Keep First, then insert Element into the tail
    typename Prepend<
      VariadicTemplate,
      TailInsertion,
      First
    >::type
  >;
};

// SortPack -----------------------------------------------------------------------------

// Forward declaration
template<
    template< typename... > class VariadicTemplate,
    typename Comparator,
    typename UnsortedList
>
struct SortPack;

// Base case: empty pack is already sorted
template< template<typename...> class VariadicTemplate, typename Comparator >
struct SortPack< VariadicTemplate, Comparator, VariadicTemplate<> >
{
  using type = VariadicTemplate<>;
};

// Sort the tail, then insert the head
template<
  template<typename...> class VariadicTemplate,
  typename Comparator,
  typename First,
  typename... Rest
>
struct SortPack< VariadicTemplate, Comparator, VariadicTemplate< First, Rest... > >
{
private:
  using SortedTail = typename SortPack< VariadicTemplate, Comparator, VariadicTemplate< Rest... > >::type;

public:
  using type = typename InsertSorted< VariadicTemplate, Comparator, SortedTail, First >::type;
};

}