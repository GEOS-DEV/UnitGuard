#include <gtest/gtest.h>
#include <type_traits>
#include "../UnitGuard.hpp"

using namespace UnitGuard;

TEST( UnitDimensionTests, BasicDimensions )
{
  // Verify that dimensionless is empty
  static_assert(std::is_same< Dimensionless, Unit< > >::value, "Dimensionless should be Unit<>");
  // Basic single-base units
  static_assert(!std::is_same< LengthDimension, Dimensionless >::value, "LengthDimension should not be dimensionless");
  static_assert(!std::is_same< MassDimension, Dimensionless >::value, "MassDimension should not be dimensionless");
  static_assert(!std::is_same< TimeDimension, Dimensionless >::value, "TimeDimension should not be dimensionless");
  static_assert(!std::is_same< TemperatureDimension, Dimensionless >::value, "TemperatureDimension should not be dimensionless");
  // Just confirm it compiles correctly
  SUCCEED();
}

TEST( UnitDimensionTests, DerivedUnits )
{
  // Check some derived units via static_assert
  // E.g. VelocityDimension = L^1 * T^-1
  using ExpectedVelocity = Unit< Power< LengthTag, 1 >, Power< TimeTag, -1 > >;
  static_assert( std::is_same< VelocityDimension, ExpectedVelocity >::value, "VelocityDimension mismatch!" );

  // ForceDimension = M^1 * L^1 * T^-2
  using ExpectedForce = Unit< Power< MassTag, 1 >, Power< LengthTag, 1 >, Power< TimeTag, -2 > >;
  static_assert( std::is_same< ForceDimension, ExpectedForce >::value, "ForceDimension mismatch!" );

  // PressureDimension = M^1 * L^-1 * T^-2
  using ExpectedPressure = Unit< Power< MassTag, 1 >, Power< LengthTag, -1 >, Power< TimeTag, -2 > >;
  static_assert( std::is_same< PressureDimension, ExpectedPressure >::value, "PressureDimension mismatch!" );

  // EnergyDimension = M^1 * L^2 * T^-2
  using ExpectedEnergy = Unit< Power< MassTag, 1 >, Power< LengthTag, 2 >, Power< TimeTag, -2 >  >;
  static_assert( std::is_same< EnergyDimension, ExpectedEnergy >::value, "EnergyDimension mismatch!" );

  // EntropyDimension = M^1 * L^2 * T^-2 * (Temperature)^-1
  using ExpectedEntropy = Unit< Power< MassTag, 1 >, Power< LengthTag, 2 >, Power< TimeTag, -2 >, Power< TemperatureTag, -1 > >;
  static_assert( std::is_same< EntropyDimension, ExpectedEntropy >::value,"EntropyDimension mismatch!" );

  SUCCEED();
}

TEST( UnitDimensionTests, Merge )
{
  // Suppose we merge "Length^1" into an empty unit:
  using T1 = MergeUnits< Unit< >, Power< LengthTag, 1 > >::type;
  // => Unit< Power<Length,1> >
  static_assert( std::is_same< T1, Unit< Power< LengthTag, 1 > > >::value );

  // Suppose we merge "Length^2" into Unit<Power<Length,1>, Power<Time,-1>>
  using U2 = Unit< Power< LengthTag, 1 >, Power< TimeTag, -1 > >;
  using T2 = MergeUnits< U2, Power< LengthTag , 2 > >::type;
  // => Summation of exponents => Length^(1+2)=3 => Unit<Power<Length,3>, Power<Time,-1>>
  static_assert( std::is_same< T2, Unit< Power< LengthTag, 3 >, Power< TimeTag, -1 > > >::value );

  // Suppose we merge "Mass^1" into that => no matching base => mass is appended
  using T3 = MergeUnits< T2, Power< MassTag, 1 > >::type;
  // => Unit<Power<Length,3>, Power<Time,-1>, Power<Mass,1>>
  static_assert( std::is_same< T3, Unit< Power< LengthTag, 3 >, Power< TimeTag, -1 >, Power< MassTag, 1 > > >::value);
}

TEST( MetafunctionTests, MultiplyAndDivide )
{
  // Multiply tests: L * T^-1 => VelocityDimension
  using Mul1 = typename Multiply< LengthDimension, Unit< Power< TimeTag, -1 > > >::type;
  static_assert( std::is_same< Mul1, VelocityDimension >::value, "Multiply L^1 by T^-1 => VelocityDimension" );

  // Divide tests: L / T^-1 => L * T^1 => L^1 * T^1
  using Div1 = typename Divide< LengthDimension, Unit< Power< TimeTag, -1 > > >::type;
  using Expected = Unit< Power< LengthTag, 1 >, Power< TimeTag, 1 > >;
  static_assert( std::is_same< Div1, Expected >::value, "Divide L^1 by T^-1 => L^1 * T^1" );

  // Inversion test: invert Temperature => Temperature^-1
  using InvTemp = typename Invert< TemperatureDimension >::type;
  static_assert( std::is_same< InvTemp, Unit< Power< TemperatureTag, -1 > > >::value, "Invert Temperature => Temperature^-1");

  SUCCEED();
}

//------------------------------------------------------------------------------
// Checking the Quantity<T, U> operations
//------------------------------------------------------------------------------
TEST( QuantityTests, BasicArithmetic )
{
  // We'll use double for T, and e.g. LengthDimension for U
  Length< double > dist1 {2.5};
  Length< double > dist2 {3.5};

  // Addition (same dimension)
  auto distSum = dist1 + dist2;
  EXPECT_DOUBLE_EQ( static_cast< double >( distSum ), 6.0 );

  // Subtraction (same dimension)
  auto distDiff = dist2 - dist1;
  EXPECT_DOUBLE_EQ( static_cast< double >( distDiff ), 1.0 );

  // Multiplying two lengths => L^2
  auto area = dist1 * dist2;
  // Check type
  static_assert( std::is_same< decltype( area ), Area< double > >::value, "Multiplying Length by Length => Area" );
  // Check value
  EXPECT_DOUBLE_EQ( static_cast< double >( area ), 2.5 * 3.5 );

  // Dividing two lengths => dimensionless
  auto ratio = dist2 / dist1;
  using RatioType = Quantity< double, Dimensionless >;
  static_assert( std::is_same< decltype(ratio), RatioType >::value, "Dividing Length by Length => Dimensionless" );
  EXPECT_DOUBLE_EQ( static_cast<double>(ratio), 3.5 / 2.5 );
}

TEST( QuantityTests, CrossDimensionArithmetic )
{
  // (Length) / (Time) => Velocity
  Length< double > len{10.0};
  Time< double > dur{2.0};

  auto speed = len / dur;
  static_assert( std::is_same< decltype( speed ), Velocity< double > >::value, "Length / Time => Velocity" );
  EXPECT_DOUBLE_EQ( static_cast< double >( speed ), 10.0 / 2.0 );

  // (Energy) / (Temperature) => Entropy
  Energy< double > energy {1000.0};
  Temp< double > temp {200.0};

  auto entropy = energy / temp; // => M^1 L^2 T^-2 / Temperature^1 => M L^2 T^-2 Temperature^-1
  static_assert( std::is_same< decltype( entropy ), Entropy< double > >::value, "Energy / Temperature => Entropy" );
  EXPECT_DOUBLE_EQ( static_cast< double >( entropy ), 1000.0 / 200.0 );
}

int main( int argc, char ** argv )
{
  ::testing::InitGoogleTest( &argc, argv );
  return RUN_ALL_TESTS();
}


