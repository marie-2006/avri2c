/* Wetterstation mit CodeVision, M. Eisele, 13.02.2025 */

/******************************************************************************/

#include <stdint.h>
#include <string.h>

#include "platform.h"

/******************************************************************************/

#define OPMODE_NORMAL   0
#define OPMODE_SETTIME  1

#define TASTE_NIX               (0)              // Tasten definieren
#define TASTE_SEK               (1)
#define TASTE_MIN               (2)
#define TASTE_H                 (3)
#define TASTE_STORE_RTC         (4)
#define TASTE_AUSGABE           (5)
#define TASTE_ZEAHLER           (6)
#define TASTE_ZEAHLER_ZURUECK   (7)

/******************************************************************************/

uint8_t  opmode = OPMODE_NORMAL;
uint16_t settime = 0;

uint8_t  tasten_feld = 0;
uint8_t  tasten_feld_prev = 0;

uint16_t time_1000ms_prev = 0;
uint16_t time_100ms_prev = 0;
uint16_t time_10ms_prev = 0;

uint8_t  zeit_sekunden_bcd = 0;
uint8_t  zeit_minuten_bcd = 0;
uint8_t  zeit_stunden_bcd = 0;

/******************************************************************************/

uint8_t z = 0;

/******************************************************************************/
/* Tasten vom Board abfragen */


uint8_t tastenabfrage (void)
{
  uint8_t tasten_neu = 0;
  uint8_t taste = TASTE_NIX;

  tasten_feld = ~PINK;

  tasten_neu = tasten_feld_prev ^ tasten_feld;
  tasten_neu &= tasten_feld;

  if      (tasten_neu & (1 << 4)) taste = TASTE_SEK;
  else if (tasten_neu & (1 << 5)) taste = TASTE_MIN;
  else if (tasten_neu & (1 << 6)) taste = TASTE_H;
  else if (tasten_neu & (1 << 7)) taste = TASTE_AUSGABE;
  else if (tasten_neu & (1 << 0)) taste = TASTE_ZEAHLER;
  else if (tasten_neu & (1 << 3)) taste = TASTE_STORE_RTC;
  else if (tasten_neu & (1 << 1)) taste = TASTE_ZEAHLER_ZURUECK;



  tasten_feld_prev = tasten_feld;
  return taste;
} /* tastenabfrage */


/******************************************************************************/
/* Zeitfunktion & Sekunden hochzählen */


void bcd_zaehler_anzeigen (uint8_t zaehler_wert)
{
  uint8_t einer =   zaehler_wert & 0x0f;
  uint8_t zehner = (zaehler_wert >> 4) & 0x0f;

  lcd_print_char ('0' + zehner);
  lcd_print_char ('0' + einer);
} /* bcd_zaehler_anzeigen */


uint8_t bcd_plus_eins (uint8_t x, uint8_t limit)
{
  x += 1;
  if ((x & 0xf) >= 0x0a) { // BCD Überlauf
    x += 0x06;
  }
  if (x >= limit) x = 0;

  return x;
} /* bcd_plus_eins */


uint8_t zeit_weiter_eine_sekunde (void)
{
  zeit_sekunden_bcd = bcd_plus_eins (zeit_sekunden_bcd, 0x60);
  if (zeit_sekunden_bcd == 0) { /* Sekunden Überlauf */
    zeit_minuten_bcd = bcd_plus_eins (zeit_minuten_bcd, 0x60);
    if (zeit_minuten_bcd == 0) { /* Miuten-Überlauf */
      zeit_stunden_bcd = bcd_plus_eins (zeit_stunden_bcd, 0x24);
    }
  }
  return 0;
} /* zeit_weiter_eine_sekunde */


/******************************************************************************/
/* Wetterdaten vom Sensor holen und anzeigen */


void wetterdaten_anzeigen ()
{
  bmp280_read ();

  /* Temperatur */
  lcd_set_cursor (0, 1);
  if (bmp280_id) {
    int32_to_text_decimal (bmp280_temp, 2);
    insert_decimal_point10 ();
    lcd_print_text (text_buffer + TEXT_BUFFER_RIGHT - 4);
    lcd_print_char (0xdf);
    lcd_print_char ('C');
  }

  /* Luftdruck */
  lcd_set_cursor (10, 2);
  if (bmp280_id) {
    int32_to_text_decimal (bmp280_pres, 2);
    insert_decimal_point10 ();
    lcd_print_text (text_buffer + TEXT_BUFFER_RIGHT - 5);
    lcd_print_text ("hPa");
  }

  /* Luftfeuchte */
  lcd_set_cursor (10, 1);
  if (bmp280_id == BME280_ID_VAL) {
    int32_to_text_decimal (bmp280_humi, 2);
    insert_decimal_point10 ();
    lcd_print_text (text_buffer + TEXT_BUFFER_RIGHT - 5);
    lcd_print_text ("%RH");
  }
} /* wetterdaten anzeigen */


/******************************************************************************/


void show_mode ()
{
  lcd_set_cursor (0, 3);
  if (opmode == OPMODE_NORMAL) {
    lcd_print_text ("A: SETTIME          ");
  }
  else if (opmode == OPMODE_SETTIME) {
    lcd_print_text (rightmost (uint32_to_text_hex (settime), 4));
    lcd_print_text (" #OK");
    lcd_print_text (" *BACK");
  }
} /* show_mode */


#if defined(__CODEVISIONAVR__)
void main (void)
#else
int main (void)
#endif
{
  opmode = OPMODE_NORMAL;

  timebase_init ();
  uart0_init (16);
  i2c_init ();

  DDRB |= (1 << 7);
  PORTB &= ~(1 << 7);

  sei ();

  lcd_init ();

  lcd_backlight_on ();

  // lcd_set_cursor (7, 1);
  //
  // lcd_print_char ('H');
  // lcd_print_char ('a');
  // lcd_print_char ('l');
  // lcd_print_char ('l');
  // lcd_print_char ('o');
  // lcd_print_char ('1');
  // lcd_print_char ('2');
  // lcd_print_char ('3');

  // lcd_set_cursor (6, 0);
  // lcd_print_text ("TIME:");

  //lcd_set_cursor (0, 0);
  //lcd_print_text ("Vogelhaus");
  //
  //lcd_set_cursor (0, 2);
  //lcd_print_text ("Temp");
  //
  //lcd_set_cursor (0, 3);
  //lcd_print_text ("Druck");

  rtc_read ();

  bmp280_start ();

  time_1000ms_prev = millis ();

  show_mode ();

  serial_print_text ("\r\n");
  serial_print_text ("\r\n");
  serial_print_text ("*** Hallo Marie ***");
  serial_print_text ("\r\n");

  while (1) {
    uint8_t  taste = 0;       // Tastencode
    uint8_t  rxchar = 0;
    uint16_t time_ms = 0;
    uint8_t  zeit_anzeigen = 0;

    taste = tastenabfrage ();



    if (taste == TASTE_SEK) {
      serial_print_text ("K: ");
      serial_print_text (uint32_to_text_hex (taste));
      serial_print_text (" -> ");
      serial_print_text (uint32_to_text (taste));
      serial_print_text ("\r\n");
    }

    if (taste == TASTE_AUSGABE) {
      serial_print_text ("Hi there!");
    }

    if (taste == TASTE_ZEAHLER) {
      serial_print_text ("z=");
      serial_print_text (  uint32_to_text (z)   );
      serial_print_text ("\r\n");
      z++;
    }

    if (taste == TASTE_ZEAHLER_ZURUECK) {
      //z = z - 1;
      serial_print_text ("z=");
      serial_print_text ( uint32_to_text (z) );
      serial_print_text ("\r\n");
      z--;
    }

    /* Uhr stellen */
    if (taste == TASTE_SEK) {
      zeit_sekunden_bcd = 0x00;
      time_1000ms_prev = time_ms;
      zeit_anzeigen = 1;
    }

    else if (taste == TASTE_MIN) {
      zeit_minuten_bcd = bcd_plus_eins (zeit_minuten_bcd, 0x60);
      zeit_anzeigen = 1;
    }

    else if (taste == TASTE_H) {
      zeit_stunden_bcd = bcd_plus_eins (zeit_stunden_bcd, 0x24);
      zeit_anzeigen = 1;
    }

    else if (taste == TASTE_STORE_RTC) {
      rtc_write ();
      z = 5;
      lcd_backlight_off ();
      serial_print_text ("rtc stored");
      //serial_print_text ( uint32_to_text (z) );
      serial_print_text ("\r\n");
    }

    time_ms = millis ();

    if ((time_ms - time_1000ms_prev) >= 1000) {
      time_1000ms_prev += 1000;

      zeit_weiter_eine_sekunde ();
      zeit_anzeigen = 1;

      wetterdaten_anzeigen ();
    }

    if (zeit_anzeigen != 0) {
      lcd_set_cursor (12, 0);
      bcd_zaehler_anzeigen (zeit_stunden_bcd);
      lcd_print_char (':');
      bcd_zaehler_anzeigen (zeit_minuten_bcd);
      lcd_print_char (':');
      bcd_zaehler_anzeigen (zeit_sekunden_bcd);

      zeit_anzeigen = 0;
    }

    if ((time_ms - time_100ms_prev) >= 100) {
      //PORTB ^= (1 << 7);
      time_100ms_prev = time_ms;

      if (z > 0) {
        serial_print_text ("z=");
        serial_print_text ( uint32_to_text (z) );
        serial_print_text ("\r\n");
        z--;

        if (z == 0) {
          lcd_backlight_on ();

          serial_print_text ("NULL");
          serial_print_text ("\r\n");
        }
      }
    }

    if ((time_ms - time_10ms_prev) >= 10) { /* alle 10ms */
      rxchar = keypad_read ();
      time_10ms_prev = time_ms;
    }

    if (rxchar == 0) {
      rxchar = uart0_rx ();
    }

    if (rxchar != 0) {

      if (opmode == OPMODE_NORMAL) {
        switch (rxchar) {

          case 'A': opmode = OPMODE_SETTIME;
                    settime = 0;
                    show_mode ();
                    break;

          case 'o': lcd_backlight_off ();
                    break;

          case 'O': lcd_backlight_on ();
                    break;

          case 'd': uart0_tx ('w');
                    mdelay_ms (1000);
                    uart0_tx ('W');
                    break;

          case 'u': uart0_tx ('u');
                    mdelay_us (100);
                    uart0_tx ('U');
                    break;

          case 'B':
          case 'm': zeit_minuten_bcd = bcd_plus_eins (zeit_minuten_bcd, 0x60);
                    zeit_anzeigen = 1;
                    break;

          case 'H':
          case 'h': zeit_stunden_bcd = bcd_plus_eins (zeit_stunden_bcd, 0x24);
                    zeit_anzeigen = 1;
                    break;

          case 'S':
          case 's': zeit_sekunden_bcd = 0x00;
                    time_1000ms_prev = time_ms;
                    zeit_anzeigen = 1;
                    break;

          case 'b': bmp280_read ();
                    break;

          case 'W': rtc_write ();
                    break;

          // case 'k': { uint16_t keycode = keypad_read ();
          //
          //             serial_print_text ("K: ");
          //             serial_print_text (uint32_to_text_hex (keycode));
          //             serial_print_text ("\r\n");
          //           }
          //           break;

          default:  uart0_tx (rxchar); /* echo */
                    break;
        }
      }
      else if (opmode == OPMODE_SETTIME) {
        switch (rxchar) {
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
                    settime <<= 4;
                    settime += rxchar - '0';
                    show_mode ();
                    break;

          case '#': { uint8_t min = 0;
                      uint8_t hour = 0;

                      min = (settime & 0xff);
                      settime >>= 8;
                      hour = (settime & 0xff);

                      if ((hour < 0x24) && (min < 0x60)) {
                        zeit_minuten_bcd = min;
                        zeit_stunden_bcd = hour;
                        zeit_sekunden_bcd = 0x00;
                        time_1000ms_prev = time_ms;

                        rtc_write ();
                      }
                    }

          case '*': opmode = OPMODE_NORMAL;
                    show_mode ();
                    break;

          default:
                    break;
        }
      }
    }
  }

#if defined(__CODEVISIONAVR__)
  /* nix */
#else
  return 0;
#endif
} /* main */


/******************************************************************************/


