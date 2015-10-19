#
# Copyright (C) 2014 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

start:
w: 2, 3, 4
t: u8, u16, u32, i8, i16, i32, f32
t: u8, u16, u32, i8, i16, i32, f32
name: convert_#3#1
arg: #2#1 v compatible(#3)
ret: #3#1
comment:
 Component wise conversion from #2#1 to #3#1
version: 9
end:

start:
w: 2, 3, 4
t: u64, i64, f64
t: u64, i64, f64
name: convert_#3#1
arg: #2#1 v compatible(#3)
ret: #3#1
comment:
 Component wise conversion from #2#1 to #3#1
version: 21
end:

start:
w: 2, 3, 4
t: u64, i64, f64
t: u8, u16, u32, i8, i16, i32, f32
name: convert_#3#1
arg: #2#1 v compatible(#3)
ret: #3#1
comment:
 Component wise conversion from #2#1 to #3#1
version: 21
end:

start:
w: 2, 3, 4
t: u8, u16, u32, i8, i16, i32, f32
t: u64, i64, f64
name: convert_#3#1
arg: #2#1 v compatible(#3)
ret: #3#1
comment:
 Component wise conversion from #2#1 to #3#1
version: 21
end:

start:
w: 1, 2, 3, 4
t: f32
name: acos
ret: #2#1
arg: #2#1 v range(-1,1)
comment:
 acos
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: acosh
ret: #2#1
arg: #2#1
comment:
 acosh
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: acospi
ret: #2#1
arg: #2#1 v range(-1,1)
comment:
 acospi
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: asin
ret: #2#1
arg: #2#1 v range(-1,1)
comment:
 asin
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: asinh
ret: #2#1
arg: #2#1
comment:
 asinh
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: asinpi
ret: #2#1
arg: #2#1 v range(-1,1)
comment:
 Return the inverse sine divided by PI.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: atan
ret: #2#1
arg: #2#1 v range(-1,1)
comment:
 Return the inverse tangent.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: atan2
ret: #2#1
arg: #2#1 y
arg: #2#1 x
comment:
 Return the inverse tangent of y / x.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: atanh
ret: #2#1
arg: #2#1 v range(-1,1)
comment:
 Return the inverse hyperbolic tangent.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: atanpi
ret: #2#1
arg: #2#1 v range(-1,1)
comment:
 Return the inverse tangent divided by PI.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: atan2pi
ret: #2#1
arg: #2#1 y
arg: #2#1 x
comment:
 Return the inverse tangent of y / x, divided by PI.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: cbrt
ret: #2#1
arg: #2#1
comment:
 Return the cube root.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: ceil
ret: #2#1
arg: #2#1
comment:
 Return the smallest integer not less than a value.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: copysign
ret: #2#1
arg: #2#1 x
arg: #2#1 y
comment:
 Copy the sign bit from y to x.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: cos
ret: #2#1
arg: #2#1
comment:
 Return the cosine.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: cosh
ret: #2#1
arg: #2#1
comment:
 Return the hypebolic cosine.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: cospi
ret: #2#1
arg: #2#1
comment:
 Return the cosine of the value * PI.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: erfc
ret: #2#1
arg: #2#1
comment:
 Return the complementary error function.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: erf
ret: #2#1
arg: #2#1
comment:
 Return the error function.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: exp
ret: #2#1
arg: #2#1
comment:
 Return e ^ value.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: exp2
ret: #2#1
arg: #2#1
comment:
 Return 2 ^ value.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: exp10
ret: #2#1
arg: #2#1
comment:
 Return 10 ^ value.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: expm1
ret: #2#1
arg: #2#1
comment:
 Return (e ^ value) - 1.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: fabs
ret: #2#1
arg: #2#1
comment:
 Return the absolute value of a value.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: fdim
ret: #2#1
arg: #2#1 a
arg: #2#1 b
comment:
 Return the positive difference between two values.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: floor
ret: #2#1
arg: #2#1
comment:
 Return the smallest integer not greater than a value.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: fma
ret: #2#1
arg: #2#1 a
arg: #2#1 b
arg: #2#1 c
comment:
 Return (a * b) + c.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
# TODO What is the difference between this and max?  Same for min.
name: fmax
ret: #2#1
arg: #2#1 x
arg: #2#1 y
comment:
 Return (x < y ? y : x)
version: 9
end:

start:
w: 2, 3, 4
t: f32
name: fmax
ret: #2#1
arg: #2#1 x
arg: #2 y
comment:
 Return (x < y ? y : x)
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: fmin
ret: #2#1
arg: #2#1 x
arg: #2#1 y
comment:
 Return (x > y ? y : x)
version: 9
end:

start:
w: 2, 3, 4
t: f32
name: fmin
ret: #2#1
arg: #2#1 x
arg: #2 y
comment:
 Return (x > y ? y : x)
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: fmod
ret: #2#1
arg: #2#1 x
arg: #2#1 y
comment:
 Return the remainder from x / y
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: fract
ret: #2#1
arg: #2#1 v
arg: #2#1 *floor
comment:
 Return fractional part of v

 @param floor  floor[0] will be set to the floor of the input value.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: fract
ret: #2#1
arg: #2#1 v
comment:
 Return fractional part of v
inline:
    #2#1 unused;
    return fract(v, &unused);
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: frexp
ret: #2#1
arg: #2#1 v
arg: int#1 *iptr
comment:
 Return the mantissa and place the exponent into iptr[0]

 @param v Supports float, float2, float3, float4.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: hypot
ret: #2#1
arg: #2#1 x
arg: #2#1 y
comment:
 Return sqrt(x*x + y*y)
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: ilogb
ret: int#1
arg: float#1
comment:
 Return the integer exponent of a value
version: 9
test: custom
end:

start:
w: 1, 2, 3, 4
name: ldexp
ret: float#1
arg: float#1 x
arg: int#1 y
comment:
 Return (x * 2^y)

 @param x Supports 1,2,3,4 components
 @param y Supports single component or matching vector.
version: 9
end:

start:
w: 2, 3, 4
name: ldexp
ret: float#1
arg: float#1 x
arg: int y
comment:
 Return (x * 2^y)

 @param x Supports 1,2,3,4 components
 @param y Supports single component or matching vector.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: lgamma
ret: #2#1
arg: #2#1
comment:
 Return the log gamma
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: lgamma
ret: #2#1
arg: #2#1 x
arg: int#1 *y
comment:
 Return the log gamma and sign
version: 9
#TODO Temporary until bionic & associated drivers are fixed
test: custom
end:

start:
w: 1, 2, 3, 4
t: f32
name: log
ret: #2#1
arg: #2#1
comment:
 Return the natural logarithm.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: log2
ret: #2#1
arg: #2#1
comment:
 Return the base 2 logarithm.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: log10
ret: #2#1
arg: #2#1
comment:
 Return the base 10 logarithm.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: log1p
ret: #2#1
arg: #2#1
comment:
 Return the natural logarithm of (v + 1.0f)
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: logb
ret: #2#1
arg: #2#1
comment:
 Compute the exponent of the value.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: mad
ret: #2#1
arg: #2#1 a
arg: #2#1 b
arg: #2#1 c
comment:
 Compute (a * b) + c
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: modf
ret: #2#1
arg: #2#1 x
arg: #2#1 *iret
comment:
 Return the integral and fractional components of a number.

 @param x Source value
 @param iret iret[0] will be set to the integral portion of the number.
 @return The floating point portion of the value.
version: 9
end:

start:
w: 1
t: f32
name: nan
ret: #2#1
arg: uint#1
comment:
 generate a nan
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: nextafter
ret: #2#1
arg: #2#1 x
arg: #2#1 y
comment:
 Return the next floating point number from x towards y.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: pow
ret: #2#1
arg: #2#1 x
arg: #2#1 y
comment:
 Return x ^ y.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: pown
ret: #2#1
arg: #2#1 x
arg: int#1 y
comment:
 Return x ^ y.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: powr
ret: #2#1
arg: #2#1 x range(0,3000)
arg: #2#1 y
comment:
 Return x ^ y.
 x must be >= 0
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: remainder
ret: #2#1
arg: #2#1 x
arg: #2#1 y
comment:
 Return round x/y to the nearest integer then compute the remainder.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: remquo
ret: #2#1
arg: #2#1 b
arg: #2#1 c
arg: int#1 *d
comment:
 Return the quotient and the remainder of b/c.  Only the sign and lowest three bits of the quotient are guaranteed to be accurate.
version: 9
test: custom
end:

start:
w: 1, 2, 3, 4
t: f32
name: rint
ret: #2#1
arg: #2#1
comment:
 Round to the nearest integral value.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: rootn
ret: #2#1
arg: #2#1 v
arg: int#1 n
comment:
 Compute the Nth root of a value.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: round
ret: #2#1
arg: #2#1
comment:
 Round to the nearest integral value.  Half values are rounded away from zero.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: rsqrt
ret: #2#1
arg: #2#1
comment:
 Return (1 / sqrt(value)).
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: sqrt
ret: #2#1
arg: #2#1
comment:
 Return the square root of a value.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: sin
ret: #2#1
arg: #2#1
comment:
 Return the sine of a value specified in radians.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: sincos
ret: #2#1
arg: #2#1 v
arg: #2#1 *cosptr
comment:
 Return the sine and cosine of a value.

 @return sine
 @param v The incoming value in radians
 @param *cosptr cosptr[0] will be set to the cosine value.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: sinh
ret: #2#1
arg: #2#1
comment:
 Return the hyperbolic sine of a value specified in radians.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: sinpi
ret: #2#1
arg: #2#1
comment:
 Return the sin(v * PI).
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: tan
ret: #2#1
arg: #2#1
comment:
 Return the tangent of a value.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: tanh
ret: #2#1
arg: #2#1
comment:
 Return the hyperbolic tangent of a value.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: tanpi
ret: #2#1
arg: #2#1
comment:
 Return tan(v * PI)
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: tgamma
ret: #2#1
arg: #2#1
comment:
 Compute the gamma function of a value.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: trunc
ret: #2#1
arg: #2#1
comment:
 ound to integral using truncation.
version: 9
end:

# int functions

start:
w: 1, 2, 3, 4
t: i8, i16, i32
name: abs
ret: u#2#1
arg: #2#1 value
comment:
 Return the absolute value of a value.
version: 9
end:

start:
w: 1, 2, 3, 4
t: u8, u16, u32, i8, i16, i32
name: clz
ret: #2#1
arg: #2#1 value
comment:
 Return the number of leading 0-bits in a value.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: min
ret: #2#1
arg: #2#1
arg: #2#1
comment:
 Return the minimum value from two arguments
version: 9
end:

start:
w: 1
t: i8 i16 i32 u8 u16 u32
name: min
ret: #2#1
arg: #2#1 v1
arg: #2#1 v2
comment:
 Return the minimum value from two arguments
inline:
 return (v1 < v2 ? v1 : v2);
version: 9 19
end:

start:
w: 2
t: i8 i16 i32 u8 u16 u32
name: min
ret: #2#1
arg: #2#1 v1
arg: #2#1 v2
comment:
 Return the minimum value from two arguments
inline:
 #2#1 tmp;
 tmp.x = (v1.x < v2.x ? v1.x : v2.x);
 tmp.y = (v1.y < v2.y ? v1.y : v2.y);
 return tmp;
version: 9 19
end:

start:
w: 3
t: i8 i16 i32 u8 u16 u32
name: min
ret: #2#1
arg: #2#1 v1
arg: #2#1 v2
comment:
 Return the minimum value from two arguments
inline:
 #2#1 tmp;
 tmp.x = (v1.x < v2.x ? v1.x : v2.x);
 tmp.y = (v1.y < v2.y ? v1.y : v2.y);
 tmp.z = (v1.z < v2.z ? v1.z : v2.z);
 return tmp;
version: 9 19
end:

start:
w: 4
t: i8 i16 i32 u8 u16 u32
name: min
ret: #2#1
arg: #2#1 v1
arg: #2#1 v2
comment:
 Return the minimum value from two arguments
inline:
 #2#1 tmp;
 tmp.x = (v1.x < v2.x ? v1.x : v2.x);
 tmp.y = (v1.y < v2.y ? v1.y : v2.y);
 tmp.z = (v1.z < v2.z ? v1.z : v2.z);
 tmp.w = (v1.w < v2.w ? v1.w : v2.w);
 return tmp;
version: 9 19
end:

start:
w: 1, 2, 3, 4
t: i8 i16 i32 i64 u8 u16 u32 u64
name: min
ret: #2#1
arg: #2#1 v1
arg: #2#1 v2
comment:
 Return the minimum value from two arguments
version: 21
end:

start:
w: 1, 2, 3, 4
t: f32
name: max
ret: #2#1
arg: #2#1
arg: #2#1
comment:
 Return the maximum value from two arguments
version: 9
end:

start:
w: 1
t: i8 i16 i32 u8 u16 u32
name: max
ret: #2#1
arg: #2#1 v1
arg: #2#1 v2
comment:
 Return the maximum value from two arguments
inline:
 return (v1 > v2 ? v1 : v2);
version: 9 19
end:

start:
w: 2
t: i8 i16 i32 u8 u16 u32
name: max
ret: #2#1
arg: #2#1 v1
arg: #2#1 v2
comment:
 Return the maximum value from two arguments
inline:
 #2#1 tmp;
 tmp.x = (v1.x > v2.x ? v1.x : v2.x);
 tmp.y = (v1.y > v2.y ? v1.y : v2.y);
 return tmp;
version: 9 19
end:

start:
w: 3
t: i8 i16 i32 u8 u16 u32
name: max
ret: #2#1
arg: #2#1 v1
arg: #2#1 v2
comment:
 Return the maximum value from two arguments
inline:
 #2#1 tmp;
 tmp.x = (v1.x > v2.x ? v1.x : v2.x);
 tmp.y = (v1.y > v2.y ? v1.y : v2.y);
 tmp.z = (v1.z > v2.z ? v1.z : v2.z);
 return tmp;
version: 9 19
end:

start:
w: 4
t: i8 i16 i32 u8 u16 u32
name: max
ret: #2#1
arg: #2#1 v1
arg: #2#1 v2
comment:
 Return the maximum value from two arguments
inline:
 #2#1 tmp;
 tmp.x = (v1.x > v2.x ? v1.x : v2.x);
 tmp.y = (v1.y > v2.y ? v1.y : v2.y);
 tmp.z = (v1.z > v2.z ? v1.z : v2.z);
 tmp.w = (v1.w > v2.w ? v1.w : v2.w);
 return tmp;
version: 9 19
end:

start:
w: 1, 2, 3, 4
t: i8 i16 i32 i64 u8 u16 u32 u64
name: max
ret: #2#1
arg: #2#1 v1
arg: #2#1 v2
comment:
 Return the maximum value from two arguments
version: 21
end:

start:
w: 1, 2, 3, 4
t: f32
name: clamp
ret: #2#1
arg: #2#1 value
arg: #2#1 min_value
arg: #2#1 max_value above(min_value)
comment:
 Clamp a value to a specified high and low bound.

 @param amount value to be clamped.  Supports 1,2,3,4 components
 @param min_value Lower bound, must be scalar or matching vector.
 @param max_value High bound, must match type of low
version: 9
end:

start:
w: 2, 3, 4
t: f32
name: clamp
ret: #2#1
arg: #2#1 value
arg: #2 min_value
arg: #2 max_value above(min_value)
comment:
 Clamp a value to a specified high and low bound.

 @param amount value to be clamped.  Supports 1,2,3,4 components
 @param min_value Lower bound, must be scalar or matching vector.
 @param max_value High bound, must match type of low
version: 9
end:

start:
w: 1, 2, 3, 4
t: u8, u16, u32, u64, i8, i16, i32, i64
name: clamp
ret: #2#1
arg: #2#1 value
arg: #2#1 min_value
arg: #2#1 max_value above(min_value)
comment:
 Clamp a value to a specified high and low bound.

 @param amount value to be clamped.  Supports 1,2,3,4 components
 @param min_value Lower bound, must be scalar or matching vector.
 @param max_value High bound, must match type of low
version: 19
end:

start:
w: 2, 3, 4
t: u8, u16, u32, u64, i8, i16, i32, i64
name: clamp
ret: #2#1
arg: #2#1 value
arg: #2 min_value
arg: #2 max_value above(min_value)
comment:
 Clamp a value to a specified high and low bound.

 @param amount value to be clamped.  Supports 1,2,3,4 components
 @param min_value Lower bound, must be scalar or matching vector.
 @param max_value High bound, must match type of low
version: 19
end:

start:
w: 1, 2, 3, 4
t: f32
name: degrees
ret: #2#1
arg: #2#1 value
comment:
 Convert from radians to degrees.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: mix
ret: #2#1
arg: #2#1 start
arg: #2#1 stop
arg: #2#1 amount
comment:
 return start + ((stop - start) * amount)
version: 9
end:

start:
w: 2, 3, 4
t: f32
name: mix
ret: #2#1
arg: #2#1 start
arg: #2#1 stop
arg: #2 amount
comment:
 return start + ((stop - start) * amount)
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: radians
ret: #2#1
arg: #2#1 value
comment:
 Convert from degrees to radians.
version: 9
end:

start:
w: 1, 2, 3, 4
t: f32
name: step
ret: #2#1
arg: #2#1 edge
arg: #2#1 v
comment:
 if (v < edge)
     return 0.f;
 else
     return 1.f;
version: 9
end:

start:
w: 2, 3, 4
t: f32
name: step
ret: #2#1
arg: #2#1 edge
arg: #2 v
comment:
 if (v < edge)
     return 0.f;
 else
     return 1.f;
version: 9
end:

start:
w: 2, 3, 4
t: f32
name: step
ret: #2#1
arg: #2 edge
arg: #2#1 v
comment:
 if (v < edge)
     return 0.f;
 else
     return 1.f;
version: 21
end:

start:
w: 1, 2, 3, 4
t: f32
name: sign
ret: #2#1
arg: #2#1 v
comment:
 Return the sign of a value.

 if (v < 0) return -1.f;
 else if (v > 0) return 1.f;
 else return 0.f;
version: 9
end:

start:
w: 3, 4
t: f32
name: cross
ret: #2#1
arg: #2#1 lhs
arg: #2#1 rhs
comment:
 Compute the cross product of two vectors.
version: 9
test: vector
end:

start:
w: 1, 2, 3, 4
t: f32
name: dot
ret: #2
arg: #2#1 lhs
arg: #2#1 rhs
comment:
 Compute the dot product of two vectors.
version: 9
test: vector
end:

start:
w: 1, 2, 3, 4
t: f32
name: length
ret: #2
arg: #2#1 v
comment:
 Compute the length of a vector.
version: 9
test: vector
end:

start:
w: 1, 2, 3, 4
t: f32
name: distance
ret: #2
arg: #2#1 lhs
arg: #2#1 rhs
comment:
 Compute the distance between two points.
version: 9
test: vector
end:

start:
w: 1, 2, 3, 4
t: f32
name: normalize
ret: #2#1
arg: #2#1 v
comment:
 Normalize a vector.
version: 9
test: vector
end:

start:
w: 1, 2, 3, 4
t: f32
name: half_recip
ret: #2#1
arg: #2#1 v
comment:
 Return the approximate reciprocal of a value.
version: 17
end:

start:
w: 1, 2, 3, 4
t: f32
name: half_sqrt
ret: #2#1
arg: #2#1 v
comment:
 Return the approximate square root of a value.
version: 17
end:

start:
w: 1, 2, 3, 4
t: f32
name: half_rsqrt
ret: #2#1
arg: #2#1 v
comment:
 Return the approximate value of (1.f / sqrt(value)).
version: 17
end:

start:
w: 1, 2, 3, 4
t: f32
name: fast_length
ret: #2
arg: #2#1 v
comment:
 Compute the approximate length of a vector.
version: 17
test: vector
end:

start:
w: 1, 2, 3, 4
t: f32
name: fast_distance
ret: #2
arg: #2#1 lhs
arg: #2#1 rhs
comment:
 Compute the approximate distance between two points.
version: 17
test: vector
end:

start:
w: 1, 2, 3, 4
t: f32
name: fast_normalize
ret: #2#1
arg: #2#1 v
comment:
 Approximately normalize a vector.
version: 17
test: vector
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_exp
ret: #2#1
arg: #2#1 v range(-86,86)
comment:
 Fast approximate exp
 valid for inputs -86.f to 86.f
 Max 8192 ulps of error
version: 18
test: limited
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_exp2
ret: #2#1
arg: #2#1 v range(-125,125)
comment:
 Fast approximate exp2
 valid for inputs -125.f to 125.f
 Max 8192 ulps of error
version: 18
test: limited
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_exp10
ret: #2#1
arg: #2#1 v range(-37,37)
comment:
 Fast approximate exp10
 valid for inputs -37.f to 37.f
 Max 8192 ulps of error
version: 18
test: limited
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_log
ret: #2#1
arg: #2#1 v range(10e-10,10e10)
comment:
 Fast approximate log
 It is not accurate for values very close to zero.
version: 18
test: limited
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_log2
ret: #2#1
arg: #2#1 v range(10e-10,10e10)
comment:
 Fast approximate log2
 It is not accurate for values very close to zero.
version: 18
test: limited
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_log10
ret: #2#1
arg: #2#1 v range(10e-10,10e10)
comment:
 Fast approximate log10
 It is not accurate for values very close to zero.
version: 18
test: limited
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_powr
ret: #2#1
arg: #2#1 v range(0,256)
arg: #2#1 y range(-15,15)
comment:
 Fast approximate v ^ y
 v must be between 0.f and 256.f
 y must be between -15.f and 15.f
 It is not accurate for values of v very close to zero.
version: 18
test: limited
end:


start:
w: 1, 2, 3, 4
t: f32
name: native_acos
ret: #2#1
arg: #2#1 v range(-1,1)
comment:
 acos
version: 21
# TODO Temporary
test: limited(0.0005)
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_acosh
ret: #2#1
arg: #2#1
comment:
 acosh
version: 21
# TODO Temporary
test: limited(0.0005)
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_acospi
ret: #2#1
arg: #2#1 v range(-1,1)
comment:
 acospi
version: 21
# TODO Temporary
test: limited(0.0005)
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_asin
ret: #2#1
arg: #2#1 v range(-1,1)
comment:
 asin
version: 21
# TODO Temporary
test: limited(0.0005)
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_asinh
ret: #2#1
arg: #2#1
comment:
 asinh
version: 21
# TODO Temporary
test: limited(0.0005)
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_asinpi
ret: #2#1
arg: #2#1 v range(-1,1)
comment:
 Return the inverse sine divided by PI.
version: 21
# TODO Temporary
test: limited(0.0005)
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_atan
ret: #2#1
arg: #2#1 v range(-1,1)
comment:
 Return the inverse tangent.
version: 21
# TODO Temporary
test: limited(0.0005)
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_atan2
ret: #2#1
arg: #2#1 y
arg: #2#1 x
comment:
 Return the inverse tangent of y / x.
version: 21
# TODO Temporary
test: limited(0.0005)
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_atanh
ret: #2#1
arg: #2#1 in range(-1,1)
comment:
 Return the inverse hyperbolic tangent.
version: 21
# TODO Temporary
test: limited(0.0005)
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_atanpi
ret: #2#1
arg: #2#1 v range(-1,1)
comment:
 Return the inverse tangent divided by PI.
version: 21
# TODO Temporary
test: limited(0.0005)
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_atan2pi
ret: #2#1
arg: #2#1 y
arg: #2#1 x
comment:
 Return the inverse tangent of y / x, divided by PI.
version: 21
# TODO Temporary
test: limited(0.0005)
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_cbrt
ret: #2#1
arg: #2#1
comment:
 Return the cube root.
version: 21
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_cos
ret: #2#1
arg: #2#1
comment:
 Return the cosine.
version: 21
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_cosh
ret: #2#1
arg: #2#1
comment:
 Return the hypebolic cosine.
version: 21
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_cospi
ret: #2#1
arg: #2#1
comment:
 Return the cosine of the value * PI.
version: 21
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_expm1
ret: #2#1
arg: #2#1
comment:
 Return (e ^ value) - 1.
version: 21
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_distance
ret: #2
arg: #2#1 lhs
arg: #2#1 rhs
comment:
 Compute the approximate distance between two points.
version: 21
test: vector
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_divide
ret: #2#1
arg: #2#1 lhs
arg: #2#1 rhs
comment:
 Compute the approximate division result of two values.
version: 21
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_hypot
ret: #2#1
arg: #2#1 x
arg: #2#1 y
comment:
 Return native_sqrt(x*x + y*y)
version: 21
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_normalize
ret: #2#1
arg: #2#1 v
comment:
 Normalize a vector.
version: 21
test: vector
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_length
ret: #2
arg: #2#1 v
comment:
 Compute the approximate length of a vector.
version: 21
test: vector
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_log1p
ret: #2#1
arg: #2#1
comment:
 Return the natural logarithm of (v + 1.0f)
version: 21
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_recip
ret: #2#1
arg: #2#1 v
comment:
 Return the approximate reciprocal of a value.
version: 21
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_rootn
ret: #2#1
arg: #2#1 v
arg: int#1 n
comment:
 Compute the Nth root of a value.
version: 21
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_rsqrt
ret: #2#1
arg: #2#1
comment:
 Return (1 / sqrt(value)).
version: 21
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_sin
ret: #2#1
arg: #2#1
comment:
 Return the sine of a value specified in radians.
version: 21
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_sincos
ret: #2#1
arg: #2#1 v
arg: #2#1 *cosptr
comment:
 Return the sine and cosine of a value.

 @return sine
 @param v The incoming value in radians
 @param *cosptr cosptr[0] will be set to the cosine value.
version: 21
# TODO Temporary
test: limited(0.0005)
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_sinh
ret: #2#1
arg: #2#1
comment:
 Return the hyperbolic sine of a value specified in radians.
version: 21
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_sinpi
ret: #2#1
arg: #2#1
comment:
 Return the sin(v * PI).
version: 21
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_sqrt
ret: #2#1
arg: #2#1
comment:
 Return the aproximate sqrt(v).
version: 21
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_tan
ret: #2#1
arg: #2#1
comment:
 Return the tangent of a value.
version: 21
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_tanh
ret: #2#1
arg: #2#1
comment:
 Return the hyperbolic tangent of a value.
version: 21
end:

start:
w: 1, 2, 3, 4
t: f32
name: native_tanpi
ret: #2#1
arg: #2#1
comment:
 Return tan(v * PI)
version: 21
end:


