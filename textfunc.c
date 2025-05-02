/* Wetterstation mit CodeVision, M. Eisele, 13.02.2025 */

/******************************************************************************/

#include <stdint.h>
#include <string.h>

#include "platform.h"

/******************************************************************************/

#define MAX_TEXT_BUFFER 25
static char text_buffer [MAX_TEXT_BUFFER];

/******************************************************************************/


char hexdigit (uint8_t value)
{
  if (value < 10) return '0' + value;
  return 'a' + value - 10;
} /* hexdigit */


// const char *uint8_to_text_hex (uint8_t value)
// {
//   text_buffer [2] = 0;
//   text_buffer [1] = hexdigit (value & 0xf);
//   value >>= 4;
//   text_buffer [0] = hexdigit (value & 0xf);
//   return text_buffer;
// } /* uint8_to_text_hex */
//
//
// const char *uint16_to_text_hex (uint16_t value)
// {
//   text_buffer [4] = 0;
//   text_buffer [3] = hexdigit (value & 0xf);
//   value >>= 4;
//   text_buffer [2] = hexdigit (value & 0xf);
//   value >>= 4;
//   text_buffer [1] = hexdigit (value & 0xf);
//   value >>= 4;
//   text_buffer [0] = hexdigit (value & 0xf);
//   return text_buffer;
// } /* uint16_to_text_hex */


char *rightmost (char *text, uint8_t n)
{
  uint8_t end = 0;

  while (text [end]) {
    end++;
  }

  while (end && n) {
    n--;
    end--;
  }

  return &text [end];
} /* rightmost */


char *uint32_to_text_hex (uint32_t value)
{
  uint8_t wp = 8;

  text_buffer [wp] = 0;

  while (wp != 0) {
    text_buffer [--wp] = hexdigit (value & 0xf);
    value >>= 4;
  }
  return text_buffer;
} /* uint32_to_text_hex */


//const char *uint16_to_text_decimal (uint16_t value)
//{
//  uint8_t wp = 5;
//
//  text_buffer [wp] = 0;
//
//  while (wp != 0) {
//    text_buffer [--wp] = '0' + (value % 10);
//    value /= 10;
//  }
//
//  while (text_buffer [wp] == '0') {
//    text_buffer [wp++] = ' ';
//  }
//
//  return text_buffer;
//} /* uint16_to_text_decimal */


// const char *int16_to_text_decimal (int16_t value)
// {
//   uint8_t wp = 6;
//   uint8_t neg = 0;
//
//   if (value < 0) {
//     neg = 1;
//     value = -value;
//   }
//
//   text_buffer [wp] = 0;
//
//   while (wp != 0) {
//     text_buffer [--wp] = '0' + (value % 10);
//     value /= 10;
//   }
//
//   while (text_buffer [wp] == '0') {
//     text_buffer [wp++] = ' ';
//   }
//
//   if (neg) text_buffer [wp - 1] = '-';
//
//   return text_buffer;
// } /* int16_to_text_decimal */


char *insert_decimal_point (char *text, uint8_t pos)
{
  uint8_t end = 0;

  while (text [end]) {
    end++;
  }

  //pos += 1;

  while (pos > 0) {
    text [end] =  text [end - 1];
    end--;
    pos--;
  }

  text [end] = '.';

  return text;
} /* insert_decimal_point */


char *int32_to_text_decimal (int32_t value, uint8_t minlen)
{
  uint8_t wp = 10;
  uint8_t neg = 0;

  if (value < 0) {
    neg = 1;
    value = -value;
  }

  text_buffer [wp] = 0;

  while (wp != 0) {
    text_buffer [--wp] = '0' + (value % 10);
    value /= 10;
    if (minlen) minlen--;
    if ((value == 0) && (minlen == 0)) {
      break;
    }
  }

  if (neg && wp) text_buffer [--wp] = '-';

  while (wp != 0) {
    text_buffer [--wp] = ' ';
  }

  // while (text_buffer [wp] == '0') {
  //   text_buffer [wp++] = ' ';
  // }

  return text_buffer;
} /* int32_to_text_decimal */


// const char *uint32_to_text_decimal (uint32_t value)
// {
//   uint8_t wp = 10;
//
//   text_buffer [wp] = 0;
//
//   while (wp != 0) {
//     text_buffer [--wp] = '0' + (value % 10);
//     value /= 10;
//   }
//
//   while (text_buffer [wp] == '0') {
//     text_buffer [wp++] = ' ';
//   }
//
//   return text_buffer;
// } /* uint32_to_text_decimal */


//char *uint8_to_text_decimal (uint8_t value)
//{
//  text_buffer [2] = 0;
//  if (value < 100) {
//    text_buffer [1] = '0' + (value % 10);
//    value /= 10;
//    text_buffer [0] = '0' + (value % 10);
//  }
//  else {
//    text_buffer [1] = 'x';
//    text_buffer [0] = 'x';
//  }
//
//  return text_buffer;
//} /* uint8_to_text_decimal */


/******************************************************************************/


