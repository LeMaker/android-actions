/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include  <ctype.h>
#include "dmStringUtil.h"
#include "dmNewDataTypesValidation.h"
#include "xpl_Logger.h"

//----------------------------------------------------------------------------------------------

typedef int                   position_t;
typedef int                   offset_t;

//----------------------------------------------------------------------------------------------

class matcher_t
{
public:
                              matcher_t( const DMString& str );
                              matcher_t( const matcher_t& matcher );
  BOOLEAN                     get_char( char& curr_char, offset_t offset ) const;
  BOOLEAN                     advance( offset_t offset );
  offset_t                    get_offset() const;
  position_t                  get_size() const;

private:
  position_t                  _position;                            
  position_t                  _initial_position;                            
  const DMString&             _str;
};

typedef offset_t (*check_format_fn_t)( matcher_t matcher );

//----------------------------------------------------------------------------------------------

matcher_t::matcher_t( const DMString& str )
  : _position( 0 ),
    _initial_position( 0 ),                      
    _str( str )
{
}

matcher_t::matcher_t( const matcher_t& matcher )
  : _position( matcher._position ),
    _initial_position( matcher._position ),                      
    _str( matcher._str )
{
}

BOOLEAN 
matcher_t::advance( offset_t offset )
{
  BOOLEAN result = FALSE;
  
  if( ( offset > 0 ) && 
      ( _position + offset <= get_size() ) )
  {
    _position += offset;
    result = TRUE;
  }
  
  return result;
}

BOOLEAN 
matcher_t::get_char( char& curr_char, offset_t offset ) const
{
  BOOLEAN result = FALSE;
  
  if( _position + offset < get_size() )
  {
    curr_char = _str[ _position + offset ];
    result = TRUE;
  }
  
  return result;
}     

offset_t 
matcher_t::get_offset() const
{
  return _position - _initial_position;                  
}

position_t 
matcher_t::get_size() const
{
  return static_cast<position_t>( _str.length() );
}

//----------------------------------------------------------------------------------------------

static
offset_t 
is_literal( matcher_t matcher, char literal )
{
  char curr_char = '\0';
  return ( matcher.get_char( curr_char, 0 ) && ( curr_char == literal ) ) ? 1 : 0;
}

static
offset_t 
is_sign( matcher_t matcher )
{
  offset_t offset = is_literal( matcher, '-' );
  
  if( !offset ) 
  {
    offset = is_literal( matcher, '+' );
  }
  
  return offset;
}

static
offset_t 
is_digit( matcher_t matcher )
{
  char curr_char = '\0';
  return ( matcher.get_char( curr_char, 0 ) && isdigit( curr_char ) ) ? 1 : 0;
}

static
offset_t 
is_number( matcher_t matcher, int min_length, int max_length )
{
  for( int i = 0; i < min_length; ++i )
  {
    if( !matcher.advance( is_digit( matcher ) ) )
    {
      return 0;
    }
  }  
  
  for( int i = min_length; i < max_length; ++i )
  {
    if( !matcher.advance( is_digit( matcher ) ) )
    {
      return matcher.get_offset();
    }
  }
  
  return matcher.get_offset();
}

static
offset_t 
is_integer_part( matcher_t matcher )
{
  return is_number( matcher, 1, 10 );
}

static
offset_t 
is_fractional_part( matcher_t matcher )
{
  if( !matcher.advance( is_literal( matcher, '.' ) ) ) return 0;
  if( !matcher.advance( is_number( matcher, 1, 10 ) ) ) return 0; 
  
  return matcher.get_offset(); 
}

static
offset_t 
is_Ee( matcher_t matcher )
{
  offset_t offset = is_literal( matcher, 'E' );
  
  if( !offset ) 
  {
    offset = is_literal( matcher, 'e' );
  }
  
  return offset;
}

static
offset_t 
is_exponent( matcher_t matcher )
{
  if( !matcher.advance( is_Ee( matcher ) ) ) return 0;

  matcher.advance( is_sign( matcher ) );
  if( !matcher.advance( is_number( matcher, 1, 10 ) ) ) return 0; 
  
  return matcher.get_offset(); 
}

static
offset_t 
is_mantissa( matcher_t matcher )
{
  matcher.advance( is_sign( matcher ) );
  
  if( !matcher.advance( is_integer_part( matcher ) ) ) return 0;
  
  matcher.advance( is_fractional_part( matcher ) );
  
  return matcher.get_offset();
}

BOOLEAN 
is_float( const DMString& str )
{
  XPL_LOG_DM_TMN_Debug(("dmNewDataTypesValidation::is_float str=%s\n", str.c_str()));
  matcher_t     matcher( str );

  if( matcher.get_size() == 0 ) return FALSE;
  if( !matcher.advance( is_mantissa( matcher ) ) ) return FALSE;
  matcher.advance( is_exponent( matcher ) );
  
  return matcher.get_offset() == str.length();
}

static
offset_t 
is_year( matcher_t matcher )
{
  return is_number( matcher, 4, 4 );
}

static
offset_t 
is_number_in_range( matcher_t matcher, int num_digits, int min_value, int max_value )
{
  offset_t offset = is_number( matcher, num_digits, num_digits );
  
  if( offset == 0 ) return 0;

  int       value = 0;
  
  for( int i = 0; i < num_digits; ++i )
  {
    char curr_char = '\0';
    
    if( !matcher.get_char( curr_char, i ) ) return 0;
    
    value = 10 * value + ( curr_char - '0' );
  }

  return ( min_value <= value ) && ( value <= max_value ) ? offset : 0;
}

static
offset_t 
is_month( matcher_t matcher )
{
  return is_number_in_range( matcher, 2, 1, 12 );
}

static
offset_t 
is_day_of_month( matcher_t matcher )
{
  return is_number_in_range( matcher, 2, 1, 31 );
}

static
offset_t 
is_day_of_year( matcher_t matcher )
{
  return is_number_in_range( matcher, 3, 1, 366 );
}

static
offset_t 
is_day_of_week( matcher_t matcher )
{
  return is_number_in_range( matcher, 1, 1, 7 );
}

static
offset_t 
is_week_number( matcher_t matcher )
{
  return is_number_in_range( matcher, 2, 1, 52 );
}

static
offset_t 
is_YYYY( matcher_t matcher )
{
  if( !matcher.advance( is_year( matcher ) ) )          return 0;

  return matcher.get_offset();
}

static
offset_t 
is_YYYY_MM_DD( matcher_t matcher )
{
  if( !matcher.advance( is_year( matcher ) ) )          return 0;
  if( !matcher.advance( is_literal( matcher, '-' ) ) )  return 0;
  if( !matcher.advance( is_month( matcher ) ) )         return 0;
  if( !matcher.advance( is_literal( matcher, '-' ) ) )  return 0;
  if( !matcher.advance( is_day_of_month( matcher ) ) )  return 0;

  return matcher.get_offset();
}

static
offset_t 
is_YYYY_MM( matcher_t matcher )
{
  if( !matcher.advance( is_year( matcher ) ) )          return 0;
  if( !matcher.advance( is_literal( matcher, '-' ) ) )  return 0;
  if( !matcher.advance( is_month( matcher ) ) )         return 0;

  return matcher.get_offset();
}

static
offset_t 
is_YYYY_DDD( matcher_t matcher )
{
  if( !matcher.advance( is_year( matcher ) ) )          return 0;
  if( !matcher.advance( is_literal( matcher, '-' ) ) )  return 0;
  if( !matcher.advance( is_day_of_year( matcher ) ) )   return 0;

  return matcher.get_offset();
}

static
offset_t 
is_YYYY_Wxx( matcher_t matcher )
{
  if( !matcher.advance( is_year( matcher ) ) )          return 0;
  if( !matcher.advance( is_literal( matcher, '-' ) ) )  return 0;
  if( !matcher.advance( is_literal( matcher, 'W' ) ) )  return 0;
  if( !matcher.advance( is_week_number( matcher ) ) )   return 0;

  return matcher.get_offset();
}

static
offset_t 
is_YYYY_Wxx_d( matcher_t matcher )
{
  if( !matcher.advance( is_year( matcher ) ) )          return 0;
  if( !matcher.advance( is_literal( matcher, '-' ) ) )  return 0;
  if( !matcher.advance( is_literal( matcher, 'W' ) ) )  return 0;
  if( !matcher.advance( is_week_number( matcher ) ) )   return 0;
  if( !matcher.advance( is_literal( matcher, '-' ) ) )  return 0;
  if( !matcher.advance( is_day_of_week( matcher ) ) )   return 0;

  return matcher.get_offset();
}

static
offset_t 
is_YYYYMMDD( matcher_t matcher )
{
  if( !matcher.advance( is_year( matcher ) ) )          return 0;
  if( !matcher.advance( is_month( matcher ) ) )         return 0;
  if( !matcher.advance( is_day_of_month( matcher ) ) )  return 0;

  return matcher.get_offset();
}

static
offset_t 
is_YYYYMM( matcher_t matcher )
{
  if( !matcher.advance( is_year( matcher ) ) )          return 0;
  if( !matcher.advance( is_month( matcher ) ) )         return 0;

  return matcher.get_offset();
}

static
offset_t 
is_YYYYDDD( matcher_t matcher )
{
  if( !matcher.advance( is_year( matcher ) ) )          return 0;
  if( !matcher.advance( is_day_of_year( matcher ) ) )   return 0;

  return matcher.get_offset();
}

static
offset_t 
is_YYYYWxx( matcher_t matcher )
{
  if( !matcher.advance( is_year( matcher ) ) )          return 0;
  if( !matcher.advance( is_literal( matcher, 'W' ) ) )  return 0;
  if( !matcher.advance( is_week_number( matcher ) ) )   return 0;

  return matcher.get_offset();
}

static
offset_t 
is_YYYYWxxd( matcher_t matcher )
{
  if( !matcher.advance( is_year( matcher ) ) )          return 0;
  if( !matcher.advance( is_literal( matcher, 'W' ) ) )  return 0;
  if( !matcher.advance( is_week_number( matcher ) ) )   return 0;
  if( !matcher.advance( is_day_of_week( matcher ) ) )   return 0;

  return matcher.get_offset();
}

static
BOOLEAN 
is_format( matcher_t matcher, check_format_fn_t fn )
{
  return (*fn)( matcher ) == matcher.get_size();
}

BOOLEAN 
is_date( const DMString& str )
{
  matcher_t     matcher( str );
  BOOLEAN          result = TRUE;
  
  if( matcher.get_size() == 0 ) return FALSE;
  
  for( ; ; )
  {
    if( is_format( matcher, is_YYYY ) )       break;
    if( is_format( matcher, is_YYYY_MM_DD ) ) break;
    if( is_format( matcher, is_YYYY_MM ) )    break;
    if( is_format( matcher, is_YYYY_DDD ) )   break;
    if( is_format( matcher, is_YYYY_Wxx ) )   break;
    if( is_format( matcher, is_YYYY_Wxx_d ) ) break;
    if( is_format( matcher, is_YYYYMMDD ) )   break;
    if( is_format( matcher, is_YYYYMM ) )     break;
    if( is_format( matcher, is_YYYYDDD ) )    break;
    if( is_format( matcher, is_YYYYWxx ) )    break;
    if( is_format( matcher, is_YYYYWxxd ) )   break;
    
    result = FALSE;
    break;
  }
  
  return result;
}

static
offset_t 
is_hours( matcher_t matcher )
{
  return is_number_in_range( matcher, 2, 0, 23 );
}

static
offset_t 
is_minutes( matcher_t matcher )
{
  return is_number_in_range( matcher, 2, 0, 59 );
}

static
offset_t 
is_seconds( matcher_t matcher )
{
  return is_number_in_range( matcher, 2, 0, 59 );
}

static
offset_t 
is_hh_mm_ss( matcher_t matcher )
{
  if( !matcher.advance( is_hours( matcher ) ) )         return 0;
  if( !matcher.advance( is_literal( matcher, ':' ) ) )  return 0;
  if( !matcher.advance( is_minutes( matcher ) ) )       return 0;
  if( !matcher.advance( is_literal( matcher, ':' ) ) )  return 0;
  if( !matcher.advance( is_seconds( matcher ) ) )       return 0;

  return matcher.get_offset();
}

static
offset_t 
is_hh_mm( matcher_t matcher )
{
  if( !matcher.advance( is_hours( matcher ) ) )         return 0;
  if( !matcher.advance( is_literal( matcher, ':' ) ) )  return 0;
  if( !matcher.advance( is_minutes( matcher ) ) )       return 0;

  return matcher.get_offset();
}

static
offset_t 
is_hhmmss( matcher_t matcher )
{
  if( !matcher.advance( is_hours( matcher ) ) )         return 0;
  if( !matcher.advance( is_minutes( matcher ) ) )       return 0;
  if( !matcher.advance( is_seconds( matcher ) ) )       return 0;

  return matcher.get_offset();
}

static
offset_t 
is_hhmm( matcher_t matcher )
{
  if( !matcher.advance( is_hours( matcher ) ) )         return 0;
  if( !matcher.advance( is_minutes( matcher ) ) )       return 0;

  return matcher.get_offset();
}

static
offset_t 
is_hh( matcher_t matcher )
{
  if( !matcher.advance( is_hours( matcher ) ) )         return 0;

  return matcher.get_offset();
}

static
offset_t 
is_hh_mm_ssZ( matcher_t matcher )
{
  if( !matcher.advance( is_hours( matcher ) ) )         return 0;
  if( !matcher.advance( is_literal( matcher, ':' ) ) )  return 0;
  if( !matcher.advance( is_minutes( matcher ) ) )       return 0;
  if( !matcher.advance( is_literal( matcher, ':' ) ) )  return 0;
  if( !matcher.advance( is_seconds( matcher ) ) )       return 0;
  if( !matcher.advance( is_literal( matcher, 'Z' ) ) )  return 0;

  return matcher.get_offset();
}

static
offset_t 
is_hh_mm_ss_utc_hh_mm( matcher_t matcher )
{
  if( !matcher.advance( is_hours( matcher ) ) )         return 0;
  if( !matcher.advance( is_literal( matcher, ':' ) ) )  return 0;
  if( !matcher.advance( is_minutes( matcher ) ) )       return 0;
  if( !matcher.advance( is_literal( matcher, ':' ) ) )  return 0;
  if( !matcher.advance( is_seconds( matcher ) ) )       return 0;
  if( !matcher.advance( is_sign( matcher ) ) )          return 0;
  if( !matcher.advance( is_hours( matcher ) ) )         return 0;
  if( !matcher.advance( is_literal( matcher, ':' ) ) )  return 0;
  if( !matcher.advance( is_minutes( matcher ) ) )       return 0;

  return matcher.get_offset();
}

BOOLEAN 
is_time( const DMString& str )
{
  matcher_t     matcher( str );
  BOOLEAN          result = TRUE;
  
  if( matcher.get_size() == 0 ) return FALSE;
  
  for( ; ; )
  {
    if( is_format( matcher, is_hh_mm_ss ) )           break;
    if( is_format( matcher, is_hh_mm ) )              break;
    if( is_format( matcher, is_hhmmss ) )             break;
    if( is_format( matcher, is_hhmm ) )               break;
    if( is_format( matcher, is_hh ) )                 break;
    if( is_format( matcher, is_hh_mm_ssZ ) )          break;
    if( is_format( matcher, is_hh_mm_ss_utc_hh_mm ) ) break;
    
    result = FALSE;
    break;
  }
  
  return result;
}

/* DM: Unit tests for is_time, is_data, is_float


struct test_t
{
  test_t( const char* pattern, BOOLEAN expeted_result )
    : _pattern( pattern ),
      _expected_result( expeted_result )
  {
  }
  
  const char*   _pattern;
  BOOLEAN       _expected_result;
};

void test_float()
{
  test_t tests[] = 
  {
    test_t( "1",              TRUE ),
    test_t( "123",            TRUE ),
    test_t( "+523",           TRUE ),
    test_t( "-66544",         TRUE ),
    test_t( "6.0",            TRUE ),
    test_t( "-64.55",         TRUE ),
    test_t( "+66.544",        TRUE ),
    test_t( "1e12",           TRUE ),
    test_t( "123e12",         TRUE ),
    test_t( "+523e-12",       TRUE ),
    test_t( "-66544e1",       TRUE ),
    test_t( "6.0E+01",        TRUE ),
    test_t( "-64.55E12",      TRUE ),
    test_t( "+66.544e-44",    TRUE ),

    test_t( "",               FALSE ),
    test_t( "+66.544e-",      FALSE ),
    test_t( "-66.544e",       FALSE ),
    test_t( "+66.",           FALSE ),
    test_t( "+x6.5",          FALSE ),
    test_t( "++6.0E+01",      FALSE ),
    test_t( "--6.0E+01",      FALSE ),
    test_t( "--6.0E+-01",     FALSE ),
    test_t( "633.45.40E+01",  FALSE ),
    test_t( "633.40E+2.1",    FALSE ),
  };
  
  for( int i = 0; i < sizeof( tests ) / sizeof( tests[ 0 ] ); ++i )
  {
    std::cout <<  ( ( is_float( tests[ i ]._pattern ) == tests[ i ]._expected_result ) 
                       ? "OK    " 
                       : "Failed" );
    std::cout << " - float( " << tests[ i ]._pattern << " )";
    std::cout << std::endl;
  }
}

void test_date()
{
  test_t tests[] = 
  {
    test_t( "2005-11-23", TRUE ),
    test_t( "1243-04",    TRUE ),
    test_t( "1990-123",   TRUE ),
    test_t( "1322-W12-3", TRUE ),
    test_t( "1600-W05",   TRUE ),
    test_t( "19121103",   TRUE ),
    test_t( "200001",     TRUE ),
    test_t( "2001222",    TRUE ),
    test_t( "000001",     TRUE ),
    test_t( "0000001",    TRUE ),
    test_t( "1555",       TRUE ),
    test_t( "2210W116",   TRUE ),
    test_t( "1203W12",    TRUE ),

    test_t( "20Z5-11-23", FALSE ),
    test_t( "205-11-23",  FALSE ),
    test_t( "2025-11-2",  FALSE ),
    test_t( "2025-11-",   FALSE ),
    test_t( "2025-1-12",  FALSE ),
    test_t( "2025--12",   FALSE ),
    test_t( "2025-00-12", FALSE ),
    test_t( "2025-44-12", FALSE ),
    test_t( "2025-11-00", FALSE ),
    test_t( "2025-11-65", FALSE ),
    test_t( "1990-0",     FALSE ),
    test_t( "1990-000",   FALSE ),
    test_t( "1990-367",   FALSE ),
    test_t( "990-367",    FALSE ),
    test_t( "123-04",     FALSE ),
    test_t( "1322-W12-0", FALSE ),
    test_t( "1322-W12-8", FALSE ),
    test_t( "1322-W00-3", FALSE ),
    test_t( "1322-W0-3",  FALSE ),
    test_t( "1322-W-3",   FALSE ),
    test_t( "1322-W66-3", FALSE ),
    test_t( "155",        FALSE ),
    test_t( "1",          FALSE ),
    test_t( "",           FALSE ),
    test_t( "15522",      FALSE ),
    test_t( "1552q",      FALSE ),
    test_t( "1552-",      FALSE ),
    test_t( "1552-W",     FALSE ),
    test_t( "1552W",      FALSE ),
    test_t( "2210W1163",  FALSE ),
    test_t( "1203w12",    FALSE ),
  };
  
  for( int i = 0; i < sizeof( tests ) / sizeof( tests[ 0 ] ); ++i )
  {
    std::cout <<  ( ( is_date( tests[ i ]._pattern ) == tests[ i ]._expected_result ) 
                       ? "OK    " 
                       : "Failed" );
    std::cout << " - date( " << tests[ i ]._pattern << " )";
    std::cout << std::endl;
  }
}

void test_time()
{
  test_t tests[] = 
  {
    test_t( "12:11:23",       TRUE ),
    test_t( "12:11",          TRUE ),
    test_t( "121123",         TRUE ),
    test_t( "1211",           TRUE ),
    test_t( "12",             TRUE ),
    test_t( "12:11:23Z",      TRUE ),
    test_t( "12:11:23+08:00", TRUE ),
    test_t( "12:11:23-11:30", TRUE ),

    test_t( "",               FALSE ),
    test_t( "1",              FALSE ),
    test_t( "Z",              FALSE ),
    test_t( "+1",             FALSE ),
    test_t( "25:11:23",       FALSE ),
    test_t( "15:81:23",       FALSE ),
    test_t( "15:11:73",       FALSE ),
    test_t( "::",             FALSE ),
    test_t( "1:2:3",          FALSE ),
    test_t( "12112",          FALSE ),
    test_t( "121",            FALSE ),
    test_t( "12:11:23+33:00", FALSE ),
    test_t( "12:11:23-08:99", FALSE ),
    test_t( "12:11:23 08:12", FALSE ),
    test_t( "12:11:23 08:12", FALSE ),
    test_t( "12:11:23-08:",   FALSE ),
    test_t( "12:11:23-08:1",  FALSE ),
    test_t( "12:11:23:08:11", FALSE ),
  };
  
  for( int i = 0; i < sizeof( tests ) / sizeof( tests[ 0 ] ); ++i )
  {
    std::cout <<  ( ( is_time( tests[ i ]._pattern ) == tests[ i ]._expected_result ) 
                       ? "OK    " 
                       : "Failed" );
    std::cout << " - time( " << tests[ i ]._pattern << " )";
    std::cout << std::endl;
  }
}

int main(int argc, char* argv[])
{
  test_float();
  test_date();
  test_time();
    
	return 0;
}
*/
