#include <gtest/gtest.h>
#include <type_traits>
#include "../ConstexprAlgorithms.hpp"

using namespace UnitGuard;

#include <gtest/gtest.h>
#include <type_traits>

struct A {};
struct B {};
struct C {};
struct D {};

// Provide a compile-time order for each
template< typename T >
struct TypeOrder;

template<>
struct TypeOrder< A > { static constexpr int value = 0; };

template<>
struct TypeOrder< B > { static constexpr int value = 1; };

template<>
struct TypeOrder< C > { static constexpr int value = 2; };

template<>
struct TypeOrder< D > { static constexpr int value = 3; };

// 1) Prepend Tests

// A comparator that checks if T1's rank < T2's rank
struct TestComparator
{
  template< typename T1, typename T2 >
  struct compare
  {
    static constexpr bool value = ( TypeOrder< T1 >::value < TypeOrder< T2 >::value );
  };
};

// A simple variadic template container to test with
template< typename... Ts >
struct TestList {};

TEST(CompileTimeSortTests, PrependEmptyTest)
{
  // Prepend A onto empty TestList<>
  using Original  = TestList<>;
  using Result    = Prepend<TestList, Original, A>::type;
  using Expected  = TestList<A>;

  static_assert(std::is_same<Result, Expected>::value, "Prepend A to empty list => TestList<A>");
  SUCCEED();
}

TEST(CompileTimeSortTests, PrependNonEmptyTest)
{
  // Prepend A onto TestList<B, C>
  using Original = TestList<B, C>;
  using Result   = Prepend<TestList, Original, A>::type;
  using Expected = TestList<A, B, C>;

  static_assert(std::is_same<Result, Expected>::value, "Prepend => TestList<A, B, C>");
  SUCCEED();
}

// 2) InsertSorted Tests

// Insert into empty
TEST(CompileTimeSortTests, InsertSortedEmptyTest)
{
  using SortedList = TestList<>;
  using Inserted   = InsertSorted<TestList, TestComparator, SortedList, C>::type;
  using Expected   = TestList<C>;

  static_assert(std::is_same<Inserted, Expected>::value, "Inserting into empty list => single-element list");
  SUCCEED();
}

// Insert into a single-element list
TEST(CompileTimeSortTests, InsertSortedSingleTest)
{
  // sorted list: TestList<A>
  // insert B => TestList<A,B>
  using SortedList = TestList<A>;
  using Inserted   = InsertSorted<TestList, TestComparator, SortedList, B>::type;
  using Expected   = TestList<A, B>;

  static_assert(std::is_same<Inserted, Expected>::value, "Insert B into TestList<A> => TestList<A,B>");
  SUCCEED();
}

TEST(CompileTimeSortTests, InsertSortedFrontTest)
{
  // sorted list: TestList<B,C>
  // insert A => A < B => TestList<A,B,C>
  using SortedList = TestList<B, C>;
  using Inserted   = InsertSorted<TestList, TestComparator, SortedList, A>::type;
  using Expected   = TestList<A, B, C>;

  static_assert(std::is_same<Inserted, Expected>::value, "Insert A => front");
  SUCCEED();
}

TEST(CompileTimeSortTests, InsertSortedMiddleTest)
{
  // sorted list: TestList<A, C>
  // insert B => A < B < C => TestList<A,B,C>
  using SortedList = TestList<A, C>;
  using Inserted   = InsertSorted<TestList, TestComparator, SortedList, B>::type;
  using Expected   = TestList<A, B, C>;

  static_assert(std::is_same<Inserted, Expected>::value, "Insert B => middle");
  SUCCEED();
}

// 3) SortPack Tests

TEST(CompileTimeSortTests, SortEmpty)
{
  using Unsorted = TestList<>;
  using Sorted   = SortPack<TestList, TestComparator, Unsorted>::type;
  static_assert(std::is_same<Sorted, TestList<>>::value, "Empty list remains empty");
  SUCCEED();
}

TEST(CompileTimeSortTests, SortSingle)
{
  using Unsorted = TestList<C>;
  using Sorted   = SortPack<TestList, TestComparator, Unsorted>::type;
  // Single element => trivially sorted
  static_assert(std::is_same<Sorted, TestList<C>>::value, "Single-element list is already sorted");
  SUCCEED();
}

TEST(CompileTimeSortTests, SortMultiple)
{
  // Let's scramble them: TestList<C, A, D, B>
  using Unsorted = TestList<C, A, D, B>;
  using Sorted   = SortPack<TestList, TestComparator, Unsorted>::type;
  // A < B < C < D
  using Expected = TestList<A, B, C, D>;

  static_assert(std::is_same<Sorted, Expected>::value, "SortPack => TestList<A, B, C, D>");
  SUCCEED();
}
