#ifndef HAL_ADC_MGMT_H
#define HAL_ADC_MGMT_H

/*==== INCLUDES ==============================================================*/
#include "types.h"
#include "cc1110.h"
//#include "hal_main.h"

/*==== CONSTS ================================================================*/

// Values used for the _settings_ in macros / functions below

// Reference voltage:
#define ADC_REF_1_25_V      0x00     // Internal 1.25V reference
#define ADC_REF_P0_7        0x40     // External reference on AIN7 pin
#define ADC_REF_AVDD        0x80     // AVDD_SOC pin
#define ADC_REF_P0_6_P0_7   0xC0     // External reference on AIN6-AIN7 differential input

// Resolution (decimation rate):
#define ADC_7_BIT           0x00     //  64 decimation rate
#define ADC_9_BIT           0x10     // 128 decimation rate
#define ADC_10_BIT          0x20     // 256 decimation rate
#define ADC_12_BIT          0x30     // 512 decimation rate

// Input channel:
#define ADC_AIN0            0x00     // single ended P0_0
#define ADC_AIN1            0x01     // single ended P0_1
#define ADC_AIN2            0x02     // single ended P0_2
#define ADC_AIN3            0x03     // single ended P0_3
#define ADC_AIN4            0x04     // single ended P0_4
#define ADC_AIN5            0x05     // single ended P0_5
#define ADC_AIN6            0x06     // single ended P0_6
#define ADC_AIN7            0x07     // single ended P0_7
#define ADC_GND             0x0C     // Ground
#define ADC_TEMP_SENS       0x0E     // on-chip temperature sensor
#define ADC_VDD_3           0x0F     // (vdd/3)

// Bit masks used for ADCCON1 ADC control 1
#define ADCCON1_ST_START    0x70     // Starting conversion
#define ADCCON1_ST_NORMAL   0x03     // Normal Operation

/*==== TYPES =================================================================*/

/*==== EXPORTS ===============================================================*/

/*==== MACROS=================================================================*/

// Macro for setting up a single conversion. If ADCCON1.STSEL = 11, using this
// macro will also start the conversion.
#define ADC_SINGLE_CONVERSION(settings) \
  do{                                   \
    ADCCON3 = (settings);               \
  }while(0)

// Macro for setting up a single conversion
#define ADC_SEQUENCE_SETUP(settings)    \
  do{                                   \
    ADCCON2 = (settings);               \
  }while(0)

// Macro for starting the ADC in continuous conversion mode
#define ADC_SAMPLE_CONTINUOUS()   \
  do {                            \
    ADCCON1 &= ~0x30;             \
    ADCCON1 |= 0x10;              \
  } while (0)

// Macro for stopping the ADC in continuous mode
#define ADC_STOP()                \
  do {                            \
    ADCCON1 |= 0x30;              \
  } while (0)

// Macro for initiating a single sample in single-conversion mode (ADCCON1.STSEL = 11).
#define ADC_SAMPLE_SINGLE()       \
  do{                             \
    ADC_STOP();                   \
    ADCCON1 |= 0x40;              \
} while (0)

// Macro for configuring the ADC to be started from T1 channel 0. (T1 ch 0 must be in compare mode!!)
#define ADC_TRIGGER_FROM_TIMER1() \
  do {                            \
    ADC_STOP();                   \
    ADCCON1 &= ~0x10;             \
  } while (0)

// Expression indicating whether a conversion is finished or not.
#define ADC_SAMPLE_READY()      (ADCCON1 & 0x80)

// Macro for setting/clearing a channel as input of the ADC
#define ADC_ENABLE_CHANNEL(ch)   ADCCFG |=  (0x01 << ch)
#define ADC_DISABLE_CHANNEL(ch)  ADCCFG &= ~(0x01 << ch)

// Macro for getting the ADC results
#define ADC_GET_VALUE( v )       GET_WORD( ADCH, ADCL, v )
	
/* Reference voltage: Internal 1.25 V,
Resolution: 12 bits,
ADC input: VDD/3 (VDD is the battery voltage) */
#define SAMPLE_BATTERY_VOLTAGE(v) \
do { \
	ADCCON2 = 0x3F; \
	ADCCON1 = 0x73; \
while(!(ADCCON1 & 0x80)); \
	v = ADCL; \
	v |= (((unsigned int)ADCH) << 8); \
} while(0)	
		


/* 
From: swra101a.pdf
Reference voltage: Internal 1.25 V,
Resolution: 12 bits,
ADC input: Temperature sensor
In this example it is assumed that a 1-point calibration has been performed in
production test and that the offset was found to be 29.75 mV */
#define SAMPLE_TEMP_SENSOR(v) \
do { \
	ADCCON2 = 0x3E; \
	ADCCON1 = 0x73; \
	while(!(ADCCON1 & 0x80)); \
	v = ADCL; \
	v |= (((unsigned int)ADCH) << 8); \
	} while(0)

#define CONST 0.61065 // (1250 / 2047)
#define OFFSET_DATASHEET 750
#define OFFSET_MEASURED_AT_25_DEGREES_CELCIUS 29.75
#define OFFSET (OFFSET_DATASHEET + OFFSET_MEASURED_AT_25_DEGREES_CELCIUS) // 779.75
#define TEMP_COEFF 2.43
	
/*==== FUNCTIONS =============================================================*/

/******************************************************************************
* @fn  halAdcSampleSingle
*
* @brief
*      This function makes the adc sample the given channel at the given
*      resolution with the given reference.
*
* Parameters:
*
* @param BYTE reference
*          The reference to compare the channel to be sampled.
*        BYTE resolution
*          The resolution to use during the sample (7, 9, 10 or 12 bit)
*        BYTE input
*          The channel to be sampled.
*
* @return INT16
*          The conversion result (rightbound; see hal_adc_mgmt.c for details)
*
******************************************************************************/
int16 halAdcSampleSingle(byte reference, byte resolution, uint8 input) {
    int16 value;

    ADC_ENABLE_CHANNEL(input);

    ADCIF = 0; // Clear the ADC flag
	
    ADC_SINGLE_CONVERSION(reference | resolution | input);
    while(!ADCIF);
    ADC_GET_VALUE( value );

    ADC_DISABLE_CHANNEL(input);

    // The variable 'value' contains 16 bits
    // but the converted value with X bit resolution is placed as
    // the X most significant bits (i.e. the bits are left bound)
    // of 16 bit ADCH:ADCL value.

    resolution >>= 3;
    if (resolution > 2) {
        // For 10 and 12 bit resolution
        resolution--;
    }

    return value >> (9 - resolution);
}


// Max ADC input voltage = reference voltage =>
// (VDD/3) max = 1.25 V => max VDD = 3.75 V
// 12 bits resolution means that max ADC value = 0x07FF = 2047 (dec)
// (the ADC value is 2’s complement)
// Battery voltage, VDD = adc value * (3.75 / 2047)
// To avoid using a float, the below function will return the battery voltage * 10
// Battery voltage * 10 = adc value * (3.75 / 2047) * 10
#define CONST_BATTERY 0.0183195 // (3.75 / 2047) * 10
int16 getBatteryVoltage(void) 
{
	int16 adcValue;
	
	SAMPLE_BATTERY_VOLTAGE(adcValue);
	// Note that the conversion result always resides in MSB section of ADCH:ADCL
	adcValue >>= 4; // Shift 4 due to 12 bits resolution
	return CONST_BATTERY * adcValue;
	//return test;
}
/*
// Refer to above macro
float getTemp(void)
{
	unsigned int adcValue;
	float outputVoltage;
	SAMPLE_TEMP_SENSOR(adcValue);
	// Note that the conversion result always resides in MSB section of ADCH:ADCL
	adcValue >>= 4; // Shift 4 due to 12 bits resolution
	outputVoltage = adcValue * CONST;
	return ((outputVoltage - OFFSET) / TEMP_COEFF);
}
*/


#endif /* HAL_ADC_MGMT_H */

/*==== END OF FILE ==========================================================*/
