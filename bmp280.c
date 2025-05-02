/* BMP280 mit CodeVision, M. Eisele, 13.02.2025 */

/******************************************************************************/

#include <stdint.h>
#include <string.h>
//#include <io.h>
//#include <i2c.h>

#include "platform.h"

/******************************************************************************/

#define BMP280_ADDR     0x76    // can be 0x77 or 0x76

// #define I2C_WRITE               0
// #define I2C_READ                1

#define BMP280_ID_REG           0xD0
//#define BMP280_ID_VAL           0x58
//#define BME280_ID_VAL           0x60

#define BMP280_CAL_REG_FIRST    0x88
#define BMP280_CAL_REG_LAST     0xA1
#define BMP280_CAL_DATA_SIZE    (BMP280_CAL_REG_LAST+1 - BMP280_CAL_REG_FIRST)

/* extra BME280 */
#define BME280_CAL_HUM_REG_FIRST 0xe1
#define BME280_CAL_HUM_REG_LAST  0xe6
#define BME280_CAL_HUM_DATA_SIZE (BME280_CAL_HUM_REG_LAST + 1 - BME280_CAL_HUM_REG_FIRST)

#define BMP280_CONTROL_HUMI_REG 0xF2
#define BMP280_STATUS_REG       0xF3
#define BMP280_CONTROL_REG      0xF4
#define BMP280_CONFIG_REG       0xF5

#define BMP280_PRES_REG         0xF7
#define BMP280_TEMP_REG         0xFA
#define BMP280_HUMI_REG         0xFD


#define BMP280_DEBUG(name, val) serial_print_text (name); \
                                serial_print_text ("="); \
                                serial_print_text (int32_to_text_decimal (val, 0)); \
                                serial_print_text ("\r\n");

/******************************************************************************/

int32_t  bmp280_temp;
uint32_t bmp280_pres;
uint32_t bmp280_humi;
uint8_t  bmp280_id;


// we create a struct for this, so that we can add support for two
// sensors on a I2C bus
static union _bmp280_cal_union {
         uint8_t bytes [BMP280_CAL_DATA_SIZE];
         struct {
           uint16_t dig_t1;
           int16_t  dig_t2;
           int16_t  dig_t3;
           uint16_t dig_p1;
           int16_t  dig_p2;
           int16_t  dig_p3;
           int16_t  dig_p4;
           int16_t  dig_p5;
           int16_t  dig_p6;
           int16_t  dig_p7;
           int16_t  dig_p8;
           int16_t  dig_p9;

           /*! Calibration coefficients for the humidity sensor (2 byte extra) */
           uint8_t dig_reserved;
           uint8_t dig_h1;
         } v;
       } bmp280_cal;


static union _bme280_cal_hum_union {
         uint8_t bytes [BME280_CAL_HUM_DATA_SIZE];
         struct {
           /*! Calibration coefficients for the humidity sensor (9 byte extra) */
           int16_t dig_h2;
           uint8_t dig_h3;
           int16_t dig_h4;
           int16_t dig_h5;
           int8_t  dig_h6;
         } v;
       } bme280_cal_hum;

/******************************************************************************/

void bmp280_write_register (/*uint8_t i2c_addr,*/ uint8_t reg_addr, uint8_t value)
{
  i2c_start ();
  //i2c_write (i2c_addr << 1);
  i2c_write ((BMP280_ADDR << 1) | I2C_WRITE);
  i2c_write (reg_addr);
  i2c_write (value);
  i2c_stop ();
} /* bmp280_write_register */


uint32_t bmp280_read_register (/*uint8_t i2c_addr,*/ uint8_t reg_addr, uint8_t nbytes)
{
  uint32_t result32 = 0;
  uint8_t regval = 0;

  if ((nbytes == 0) || (nbytes > 4)) return result32;

  i2c_start ();
  i2c_write ((BMP280_ADDR << 1) | I2C_WRITE);
  //i2c_write (i2c_addr << 1);
  i2c_write (reg_addr);
  i2c_stop ();

  i2c_start ();
  //i2c_write ((i2c_addr << 1) | 1);
  i2c_write((BMP280_ADDR << 1) | I2C_READ);

  while (nbytes > 0) {
    uint8_t ack = 1;

    if (nbytes == 1) ack = 0;

    regval = i2c_read (ack);

    result32 <<= 8;
    result32 |= regval;

    nbytes -= 1;
  }

  i2c_stop ();

  return result32;
} /* bmp280_read_register */


void bmp280_readmem (uint8_t reg, uint8_t buff[], uint8_t bytes)
{
  uint8_t i = 0;

  i2c_start ();
  i2c_write ((BMP280_ADDR << 1) | I2C_WRITE);
  i2c_write (reg);
  i2c_stop ();

  i2c_start ();
  i2c_write ((BMP280_ADDR << 1) | I2C_READ);

  for (i = 0; i < bytes; i++) {
    uint8_t ack = 1;
    if (i == (bytes - 1)) ack = 0;
    buff[i] = i2c_read (ack);
  }

  i2c_stop();
} /* bmp280_readmem */


// static void bmp280_getcalibration (void)
// {
//   //memset (bmp280_cal.bytes, 0, sizeof (bmp280_cal));
//
//   bmp280_readmem (BMP280_CAL_REG_FIRST,
//                   bmp280_cal.bytes,
//                   BMP280_CAL_DATA_SIZE);
//
//   //BMP280_DEBUG("T1", bmp280_cal.v.dig_t1);
//   //BMP280_DEBUG("T2", bmp280_cal.v.dig_t2);
//   //BMP280_DEBUG("T3", bmp280_cal.v.dig_t3);
//
//   //BMP280_DEBUG("P1", bmp280_cal.v.dig_p1);
//   //BMP280_DEBUG("P2", bmp280_cal.v.dig_p2);
//   //BMP280_DEBUG("P3", bmp280_cal.v.dig_p3);
//   //BMP280_DEBUG("P4", bmp280_cal.v.dig_p4);
//   //BMP280_DEBUG("P5", bmp280_cal.v.dig_p5);
//   //BMP280_DEBUG("P6", bmp280_cal.v.dig_p6);
//   //BMP280_DEBUG("P7", bmp280_cal.v.dig_p7);
//   //BMP280_DEBUG("P8", bmp280_cal.v.dig_p8);
//   //BMP280_DEBUG("P9", bmp280_cal.v.dig_p9);
// } /* bmp280_getcalibration */


void bmp280_set_ctrl_hum (uint8_t osrs_h)
{
  bmp280_write_register (BMP280_CONTROL_HUMI_REG, (osrs_h & 0x7));
} /* bmp280_set_ctrl_hum */


void bmp280_set_ctrl (uint8_t osrs_t, uint8_t osrs_p, uint8_t mode)
{
  bmp280_write_register (BMP280_CONTROL_REG,
                      ((osrs_t & 0x7) << 5)
                    | ((osrs_p & 0x7) << 2)
                    | (mode & 0x3)
        );
} /* bmp280_set_ctrl */


void bmp280_set_config (uint8_t t_sb, uint8_t filter, uint8_t spi3w_en)
{
  bmp280_write_register (BMP280_CONFIG_REG,
                            ((t_sb & 0x7) << 5)
                         | ((filter & 0x7) << 2)
                         | (spi3w_en & 1)
        );
} /* bmp280_set_config */


void bmp280_start (void)
{
  uint8_t mode = 0;

  mode |= 3; /* normal mode */
  mode |= 1 << 2; /* osrs_p = 1 -> 16bit */
  mode |= 1 << 5; /* osrs_r = 1 -> 16bit */

  bmp280_id = bmp280_read_register (BMP280_ID_REG, 1); /* ID @d8 */

  if ((bmp280_id != BMP280_ID_VAL) && (bmp280_id != BME280_ID_VAL)) {
    bmp280_id = 0;
    return;
  }

  bmp280_write_register (0xf4, mode);

  bmp280_readmem (BMP280_CAL_REG_FIRST,
                  bmp280_cal.bytes,
                  BMP280_CAL_DATA_SIZE);

  if (bmp280_id == BME280_ID_VAL) {
    static uint8_t bytes [BME280_CAL_HUM_DATA_SIZE];

    bmp280_readmem (BME280_CAL_HUM_REG_FIRST,
                    bytes,
                    BME280_CAL_HUM_DATA_SIZE);

    bme280_cal_hum.v.dig_h2 = bytes [1]; /* E2 */
    bme280_cal_hum.v.dig_h2 <<= 8;
    bme280_cal_hum.v.dig_h2 |= bytes [0]; /* E1 */

    bme280_cal_hum.v.dig_h3 = bytes [2]; /* E3 */

    bme280_cal_hum.v.dig_h4 = bytes [3]; /* E4 */
    bme280_cal_hum.v.dig_h4 <<= 4;
    bme280_cal_hum.v.dig_h4 |= bytes [4] & 0x0f; /* E5 */

    bme280_cal_hum.v.dig_h5 = bytes [5]; /* E6 */
    bme280_cal_hum.v.dig_h5 <<= 4;
    bme280_cal_hum.v.dig_h5 |= (bytes [4] >> 4) & 0x0f; /* E5 */

    bme280_cal_hum.v.dig_h6 = bytes [5]; /* E7 */
#if 0

    BMP280_DEBUG("H1", bmp280_cal.v.dig_h1);

    BMP280_DEBUG("H2", bme280_cal_hum.v.dig_h2);
    BMP280_DEBUG("H3", bme280_cal_hum.v.dig_h3);
    BMP280_DEBUG("H4", bme280_cal_hum.v.dig_h4);
    BMP280_DEBUG("H5", bme280_cal_hum.v.dig_h5);
    BMP280_DEBUG("H6", bme280_cal_hum.v.dig_h6);
#endif
  }

  if (bmp280_id == BME280_ID_VAL) {
    bmp280_set_ctrl_hum (5); /* oversampling * 16 */
  }

  bmp280_set_config (0, 4, 0); // 0.5 ms delay, 16x filter, no 3-wire SPI
  bmp280_set_ctrl (2, 5, 3); // T oversample x2, P over x2, normal mode


#if 0
static void parse_humidity_calib_data(const uint8_t *reg_data, struct bme280_dev *dev)
{
    struct bme280_calib_data *calib_data = &dev->calib_data;
    int16_t dig_h4_lsb;
    int16_t dig_h4_msb;
    int16_t dig_h5_lsb;
    int16_t dig_h5_msb;


    calib_data->dig_h2 = (int16_t)BME280_CONCAT_BYTES(reg_data[1], reg_data[0]);

    calib_data->dig_h3 = reg_data[2];

    dig_h4_msb = (int16_t)(int8_t)reg_data[3] * 16;
    dig_h4_lsb = (int16_t)(reg_data[4] & 0x0F);

    calib_data->dig_h4 = dig_h4_msb | dig_h4_lsb;

    dig_h5_msb = (int16_t)(int8_t)reg_data[5] * 16;
    dig_h5_lsb = (int16_t)(reg_data[4] >> 4);

    calib_data->dig_h5 = dig_h5_msb | dig_h5_lsb;
    calib_data->dig_h6 = (int8_t)reg_data[6];
#endif



} /* bmp280_start  */


//uint8_t bmp280_get_status (void)
//{
//  uint8_t data[1];
//  bmp280_readmem (BMP280_STATUS_REG, data, 1);
//  //BMP280_DEBUG ("Status", data[0]);
//  return data[0];
//}


void bmp280_read (void)
{
  uint32_t temp_raw = 0;
  uint32_t pres_raw = 0;
  uint32_t humi_raw = 0;
  //uint8_t  id = 0x00;
  int32_t  var1, var2, t_fine;//, x;

  //bmp280_ok = 1;

  // bmp280_id  = bmp280_read_register (BMP280_ID_REG, 1); /* ID @d8 */

  if ((bmp280_id != BMP280_ID_VAL) && (bmp280_id != BME280_ID_VAL)) {
    bmp280_temp = 0;
    bmp280_pres = 0;
    bmp280_humi = 0;
    return;
  }

  temp_raw = bmp280_read_register (0xfa, 3); /* temp */
  pres_raw = bmp280_read_register (0xf7, 3); /* press */

  if (bmp280_id == BME280_ID_VAL) {
    humi_raw = bmp280_read_register (0xfd, 2); /* humidity */

    // serial_print_text ("H:");
    // serial_print_text (uint32_to_text_hex (humi_raw));
    // serial_print_text ("\r\n");
  }

  temp_raw >>= 4;
  pres_raw >>= 4;

  // serial_print_text ("B:");
  // serial_print_text (uint8_to_text_hex (id));
  // serial_print_text (", ");
  // serial_print_text (uint32_to_text_decimal (temp_raw));
  // serial_print_text (", ");
  // serial_print_text (uint32_to_text_decimal (pres_raw));
  // serial_print_text (", ");
  // serial_print_text (uint32_to_text_hex (humi_raw));
  // serial_print_text ("\r\n");

  /* The following code is based on a 32-bit integer code from the BMP280 datasheet */

  /* compute the temperature */
#if 0
  var1 = ((((temp_raw >> 3) - ((int32_t)bmp280_cal.v.dig_t1 << 1)))
          * ((int32_t)bmp280_cal.v.dig_t2)); // >> 11;

  if (var1 < 0) {
    var1 = -var1;
    var1 >>= 11;
    var1 = -var1;
  }
  else {
    var1 >>= 11;
  }

  var2 = ((temp_raw >> 4) - ((int32_t)bmp280_cal.v.dig_t1));
  var2 *= var2;
  var2 >>= 12;
  var2 *= ((int32_t)bmp280_cal.v.dig_t3);

  //var2 = (((((temp_raw >> 4) - ((int32_t)bmp280_cal.v.dig_t1))
  //        * ((temp_raw >> 4) - ((int32_t)bmp280_cal.v.dig_t1))) >> 12)
  //        * ((int32_t)bmp280_cal.v.dig_t3));
  //        //* ((int32_t)bmp280_cal.v.dig_t3)) >> 14;

  if (var2 < 0) {
    var2 = -var2;
    var2 >>= 14;
    var2 = -var2;
  }
  else {
    var2 >>= 14;
  }

  t_fine = var1 + var2;

  bmp280_temp = (t_fine * 5 + 128) >> 8;

#else
  { // int32_t var1;
    // int32_t var2;
    // int32_t temperature;
    int32_t temperature_min = -4000;
    int32_t temperature_max = 8500;

    var1 = (int32_t)((temp_raw / 8) - ((int32_t)bmp280_cal.v.dig_t1 * 2));
    var1 = (var1 * ((int32_t)bmp280_cal.v.dig_t2)) / 2048;
    var2 = (int32_t)((temp_raw / 16) - ((int32_t)bmp280_cal.v.dig_t1));
    var2 = (((var2 * var2) / 4096) * ((int32_t)bmp280_cal.v.dig_t3)) / 16384;
    t_fine = var1 + var2;
    bmp280_temp = (t_fine * 5 + 128) / 256;

    if (bmp280_temp < temperature_min) {
      bmp280_temp = temperature_min;
    }
    else if (bmp280_temp > temperature_max) {
      bmp280_temp = temperature_max;
    }
  }


//  { double var1;
//    double var2;
//    double temperature;
//    double temperature_min = -40;
//    double temperature_max = 85;
//
//    var1 = (((double)temp_raw) / 16384.0 - ((double)bmp280_cal.v.dig_t1) / 1024.0);
//    var1 = var1 * ((double)bmp280_cal.v.dig_t2);
//
//    var2 = (((double)temp_raw) / 131072.0 - ((double)bmp280_cal.v.dig_t1) / 8192.0);
//    var2 = (var2 * var2) * ((double)bmp280_cal.v.dig_t3);
//
//    t_fine = (int32_t)(var1 + var2);
//
//    temperature = (var1 + var2) / 5120.0;
//
//    if (temperature < temperature_min) {
//      temperature = temperature_min;
//    }
//    else if (temperature > temperature_max) {
//      temperature = temperature_max;
//    }
//
//    bmp280_temp = (int32_t) (temperature * 100.0 + 0.5);
//  }











#endif





   //serial_print_text ("t=");
   //serial_print_text (int32_to_text_decimal (bmp280_temp));
   //serial_print_text ("\r\n");

  // compute the pressure
  var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
  var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)bmp280_cal.v.dig_p6);
  var2 = var2 + ((var1 * ((int32_t)bmp280_cal.v.dig_p5)) << 1);
  var2 = (var2 >> 2) + (((int32_t)bmp280_cal.v.dig_p4) << 16);
  var1 = (((bmp280_cal.v.dig_p3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3)
          + ((((int32_t)bmp280_cal.v.dig_p2) * var1) >> 1)) >> 18;
  var1 = ((((32768 + var1)) * ((int32_t)bmp280_cal.v.dig_p1)) >> 15);

  if (var1 == 0) {
    bmp280_pres = 0;
  }
  else {
    bmp280_pres = (((uint32_t)(((int32_t)1048576)-pres_raw) - (var2 >> 12))) * 3125;
    if (bmp280_pres < 0x80000000) {
      bmp280_pres = (bmp280_pres << 1) / ((uint32_t)var1);
    }
    else {
      bmp280_pres = (bmp280_pres / (uint32_t)var1) * 2;
    }
    var1 = (((int32_t)bmp280_cal.v.dig_p9) * ((int32_t)(((bmp280_pres>>3) * (bmp280_pres >> 3)) >> 13))) >> 12;
    var2 = (((int32_t)(bmp280_pres >> 2)) * ((int32_t)bmp280_cal.v.dig_p8)) >> 13;
    bmp280_pres = (uint32_t)((int32_t)bmp280_pres + ((var1 + var2 + bmp280_cal.v.dig_p7) >> 4));
  }

  // serial_print_text ("p =");
  // serial_print_text (int32_to_text_decimal (bmp280_pres, 0));
  // serial_print_text ("\r\n");


#if 0
    int64_t var1;
    int64_t var2;
    int64_t var3;
    int64_t var4;
    uint32_t pressure;
    uint32_t pressure_min = 3000000;
    uint32_t pressure_max = 11000000;

    var1 = ((int64_t)calib_data->t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib_data->dig_p6;
    var2 = var2 + ((var1 * (int64_t)calib_data->dig_p5) * 131072);
    var2 = var2 + (((int64_t)calib_data->dig_p4) * 34359738368);
    var1 = ((var1 * var1 * (int64_t)calib_data->dig_p3) / 256) + ((var1 * ((int64_t)calib_data->dig_p2) * 4096));
    var3 = ((int64_t)1) * 140737488355328;
    var1 = (var3 + var1) * ((int64_t)calib_data->dig_p1) / 8589934592;

    /* To avoid divide by zero exception */
    if (var1 != 0) {
        var4 = 1048576 - uncomp_data->pressure;
        var4 = (((var4 * INT64_C(2147483648)) - var2) * 3125) / var1;
        var1 = (((int64_t)calib_data->dig_p9) * (var4 / 8192) * (var4 / 8192)) / 33554432;
        var2 = (((int64_t)calib_data->dig_p8) * var4) / 524288;
        var4 = ((var4 + var1 + var2) / 256) + (((int64_t)calib_data->dig_p7) * 16);
        pressure = (uint32_t)(((var4 / 2) * 100) / 128);

        if (pressure < pressure_min)
        {
            pressure = pressure_min;
        }
        else if (pressure > pressure_max)
        {
            pressure = pressure_max;
        }
    }
    else
    {
        pressure = pressure_min;
    }
#endif







  if (bmp280_id == BME280_ID_VAL) {
    int32_t var1;
    int32_t var2;
    int32_t var3;
    int32_t var4;
    int32_t var5;
    //uint32_t humidity;
    uint32_t humidity_max = 102400;

    var1 = t_fine - ((int32_t)76800);
    var2 = (int32_t)(humi_raw * 16384);
    var3 = (int32_t)(((int32_t)bme280_cal_hum.v.dig_h4) * 1048576);
    var4 = ((int32_t)bme280_cal_hum.v.dig_h5) * var1;
    var5 = (((var2 - var3) - var4) + (int32_t)16384) / 32768;
    var2 = (var1 * ((int32_t)bme280_cal_hum.v.dig_h6)) / 1024;
    var3 = (var1 * ((int32_t)bme280_cal_hum.v.dig_h3)) / 2048;
    var4 = ((var2 * (var3 + (int32_t)32768)) / 1024) + (int32_t)2097152;
    var2 = ((var4 * ((int32_t)bme280_cal_hum.v.dig_h2)) + 8192) / 16384;
    var3 = var5 * var2;
    var4 = ((var3 / 32768) * (var3 / 32768)) / 128;
    var5 = var3 - ((var4 * ((int32_t)bmp280_cal.v.dig_h1)) / 16);
    var5 = (var5 < 0 ? 0 : var5);
    var5 = (var5 > 419430400 ? 419430400 : var5);
    bmp280_humi = (uint32_t)(var5 / 4096);

    if (bmp280_humi > humidity_max) {
      bmp280_humi = humidity_max;
    }

    bmp280_humi *= 1000;
    bmp280_humi /= 1024;

    // serial_print_text ("h =");
    // serial_print_text (int32_to_text_decimal (bmp280_humi, 0));
    // serial_print_text (", ");
    // serial_print_text (insert_decimal_point (int32_to_text_decimal (bmp280_humi, 4), 3));
    // serial_print_text ("\r\n");
  }
} /* bmp280_read */


/******************************************************************************/


