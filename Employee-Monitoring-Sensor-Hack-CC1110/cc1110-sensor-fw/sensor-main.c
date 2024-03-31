#include "cc1110.h"
#include "ioCCxx10_bitdef.h"
#include "cc1110_radio.h"
#include "hal_adc_mgmt.h"


/***************************************************************************/		
// Reverse enginerring findings:

// P0_7 / AIN7 	-> Nothing
// P0_6 / AIN6  -> Thermistor (small 4pin detector circle -  ambient room temperature)
// P0_2 to P0_5 -> Nothing
// P0_1 / AIN1  -> Thermopile IR small 4pin detector circle - IR radiation temp sensor)
//								 Produces a value from 0 to 1500. Need to figure out the calculation to convert this to degrees celcius.	
// P0_0 				-> PIR movement sensor. PIR sensors oscillate the voltage as objects moves across path. In this case the 12 bit ADC value 
//								 oscellates from the mid-point value of about 10000 (so between 700 to 1300).
								

/* Get ADC values of each sensor. Beacause the battery voltage is
* used as the reference to the analog value read. It means that when
* the battery voltage is lower, the voltage input digital value (ADC) value
* is increased.
*/

/***************************************************************************/		


// Bit masks to check SLEEP register
#define SLEEP_XOSC_STB_BM   0x40  // bit mask, check the stability of XOSC
#define SLEEP_HFRC_STB_BM   0x20  // bit maks, check the stability of the High-frequency RC oscillator
#define SLEEP_OSC_PD_BM     0x04  // bit mask, power down system clock oscillator(s)

#define POWER_MODE_0  0x00  // Clock oscillators on, voltage regulator on
#define POWER_MODE_1  0x01  // 32.768 KHz oscillator on, voltage regulator on
#define POWER_MODE_2  0x02  // 32.768 KHz oscillator on, voltage regulator off
#define POWER_MODE_3  0x03  // All clock oscillators off, voltage regulator off


/*******************************************************************************
 * MACROS
 */

// Macro for checking status of the high frequency RC oscillator.
#define IS_HFRC_STABLE()    (SLEEP & SLEEP_HFRC_STB_BM)

// Macro for checking status of the crystal oscillator
#define IS_XOSC_STABLE()    (SLEEP & SLEEP_XOSC_STB_BM)


/***********************************************************************************
* LOCAL VARIABLES
*/

// Variable for active mode duration
static int xdata activeModeCnt, val2 = 0;
#define ACT_MODE_TIME  10000

// Initialization of source buffers and DMA descriptor for the DMA transfer
// (ref. CC111xFx/CC251xFx Errata Note)
static unsigned char xdata PM2_BUF[7] = {0x06,0x06,0x06,0x06,0x06,0x06,0x04};
static unsigned char xdata dmaDesc[8] = {0x00,0x00,0xDF,0xBE,0x00,0x07,0x20,0x42};

static char EVENT0_HIGH = 0xFF;
static char EVENT0_LOW = 0xFF;

volatile unsigned char storedDescHigh, storedDescLow;
volatile char temp, temp2;

uint8 adc_seq  = 0;
uint8 counter  = 0;

// Sensor output
int16 battery_voltage = 0;
static unsigned char xdata payload_format[] = "V|%02d|D|%06d|%06d|%06d"; 
int16 xdata adc_results[3]   			   = {0,0,0};


/***********************************************************************************
* LOCAL FUNCTIONS
*/

/***********************************************************************************
* @fn          setup_sleep_interrupt
*
* @brief       Function which sets up the Sleep Timer Interrupt
*              for Power Mode 2 usage.
*/

void setup_sleep_interrupt(void)
{
    // Clear Sleep Timer CPU Interrupt flag (IRCON.STIF = 0)
    STIF = 0;

    // Clear Sleep Timer Module Interrupt Flag (WORIRQ.EVENT0_FLAG = 0)
    WORIRQ &= ~WORIRQ_EVENT0_FLAG;

    // Enable Sleep Timer Module Interrupt (WORIRQ.EVENT0_MASK = 1)
    WORIRQ |= WORIRQ_EVENT0_MASK;

    // Enable Sleep Timer CPU Interrupt (IEN0.STIE = 1)
    STIE = 1;

    // Enable Global Interrupt (IEN0.EA = 1)
    EA = 1;
	
}


/***********************************************************************************
* @fn          sleep_timer_isr
*
* @brief       Sleep Timer Interrupt Service Routine, which executes when
*              the Sleep Timer expires. Note that the [SLEEP.MODE] bits must
*              be cleared inside this ISR in order to prevent unintentional
*              Power Mode 2 entry.
*/
INTERRUPT(sleep_timer_isr, ST_VECTOR) // use compiler.h macro
//void sleep_timer_isr(void) interrupt ST_VECTOR
{
	
    // Clear Sleep Timer CPU interrupt flag (IRCON.STIF = 0)
    STIF = 0;

    // Clear Sleep Timer Module Interrupt Flag (WORIRQ.EVENT0_FLAG = 0)
    WORIRQ &= ~WORIRQ_EVENT0_FLAG;

    // Clear the [SLEEP.MODE] bits, because an interrupt can also occur
    // before the SoC has actually entered Power Mode 2.
	  // Note: Not required when resuming from PM0; Clear SLEEP.MODE[1:0]
    SLEEP &= ~SLEEP_MODE;
}




/***********************************************************************************
* @fn          main
*
* @brief       Enter Power Mode 2 based on CC111xFx/CC251xFx Errata Note,
*              exit Power Mode 2 using Sleep Timer Interrupt.
*/


void main(void)
{
    storedDescHigh = DMA0CFGH;
    storedDescLow  = DMA0CFGL;

    // binary port setting of 0000011 (P1_0 and P1_1 are OUTPUT mode, the rest are input)
    P1DIR |= 0x03;	
    P1_0 = 0; P1_1 = 0;
	
	
    // Setup + enable the Sleep Timer Interrupt, which is
    // intended to wake-up the SoC from Power Mode 2.
    setup_sleep_interrupt();



    // Infinite loop:
    // Enter/exit Power Mode 2.
    while(1)
    {		
				P1_1 ^= 1; // red led
			
			/***************************************************************************
			 * High Speed Crystal Oscillator (HS XOSC) @ 26Mhz           (clk_xosc.c)
			 *
			 * > Clock must be 26Mhz to be able to use radio
			 * > Select HS XOSC as system clock source and set the clockspeed to 26 Mhz.
			 *   Once the clock source change has been initiated, the clock source should
			 *   not be changed/updated again until the current clock change has finished. 
			 */
			
				// Set the system clock source to HS XOSC and max CPU speed,
				// ref. [clk]=>[clk_xosc.c]
				SLEEP &= ~SLEEP_OSC_PD; // Power up unused oscillator (HS XOSC).
				while( !(SLEEP & SLEEP_XOSC_S) ); // Wait until the HS XOSC is stable. / <<--- XOSC aka 'HS XOSC'!!
				CLKCON = (CLKCON & ~(CLKCON_CLKSPD | CLKCON_OSC)) | CLKSPD_DIV_1; // Change the system clock source to HS XOSC and set the clock speed to 26 MHz.
				while (CLKCON & CLKCON_OSC); // Wait until system clock source has changed to HS XOSC (CLKCON.OSC = 0).
				
				while (!IS_XOSC_STABLE() );
			
			  // Configure radio
			  radio_start();		

			
			
									
				// Do measurements
			  battery_voltage = getBatteryVoltage();
				
				adc_results[0] = halAdcSampleSingle(ADC_REF_AVDD, ADC_10_BIT, ADC_AIN0);  // PIR
				adc_results[1] = halAdcSampleSingle(ADC_REF_AVDD, ADC_10_BIT, ADC_AIN1);  // Directional IR Sensor (Thermopile)
				adc_results[2] = halAdcSampleSingle(ADC_REF_AVDD, ADC_10_BIT, ADC_AIN6);  // Room Temp (Thermistor)
				


				//
				// Clean up the buffer - flush and set everything to null.
				//
				// Make sure XRAM memory sections are provided as part of SDCC compile otherwise this memset xdata stuff will cause the program to do super weird failures.
				// --code-loc 0x000 --code-size 0x8000 --xram-loc 0xf000 --xram-size 0x300 --iram-size 0x100 --model-small --opt-code-speed sensor-main.c
				memset(packet, '\0', sizeof(packet));	
				memcpy(packet, packet_header, sizeof(packet_header)/sizeof(uint8)); // Header				
	

			  // The payload to send
				
			  sprintf( packet+(sizeof(packet_header)/sizeof(uint8)), (unsigned char *)&payload_format,
					battery_voltage,
					adc_results[0],
					adc_results[1],	
					adc_results[2]);

				// V|33|D|000907|000393|000138
				// V|33|D|000902|000393|000138


				send_packet();


			  P1_1 ^= 1; // red led off   
	
				
			
			 // Now... 
       // ...go back to sleep
			 
			/***************************************************************************
			 * High speed RC oscillator (HS RCOSC) @ XX Mhz      
			 * > Need to have this active before we can setup the power mode.			 
			 */

        // Power down the HS RCOSC, since it is not beeing used.
        // Note that the HS RCOSC should not be powered down before the applied
        // system clock source is stable (SLEEP.XOSC_STB = 1).			
				SLEEP |= SLEEP_OSC_PD;			
			
			
        // Switch system clock source to HS RCOSC and max CPU speed:
        // Note that this is critical for Power Mode 2. After reset or
        // exiting Power Mode 2 the system clock source is HS RCOSC,
        // but to emphasize the requirement we choose to be explicit here.
        SLEEP &= ~SLEEP_OSC_PD;
        while( !(SLEEP & SLEEP_HFRC_S) ); // Wait until the HS RCOSC  is stable. // <<--- RCOSC aka 'HFRC'!!
			
				// change system clock source to HS RCOSC and set max CPU clock speed (CLKCON.CLKSPD = 1)
				CLKCON = (CLKCON & ~CLKCON_CLKSPD) | CLKCON_OSC | CLKCON_CLKSPD0;
			
				// Wait until system clock source has actually changed (CLKCON.OSC = 1)			
        while ( !(CLKCON & CLKCON_OSC) ) ;
				
				// Check stability
				while (!IS_HFRC_STABLE() );				
				
				// Power down [HS XOSC] (SLEEP.OSC_PD = 1)
        SLEEP |= SLEEP_OSC_PD; 

				// Low power RCOSC 32kHz set; the HS RCOSC must be the clock source to change this
				CLKCON |= CLKCON_OSC32; 

				while (!(CLKCON & CLKCON_OSC32)); // Wait until the low power RC0SC 32kHz clock has been set.			

        // Wait some time in Active Mode, and set LED before
        // entering Power Mode 2
        //for(activeModeCnt = 0; activeModeCnt < ACT_MODE_TIME; activeModeCnt++);
			

        ///////////////////////////////////////////////////////////////////////
        ////////// CC111xFx/CC251xFx Errata Note Code section Begin ///////////
        ///////////////////////////////////////////////////////////////////////

        // Store current DMA channel 0 descriptor and abort any ongoing transfers,
        // if the channel is in use.
        storedDescHigh = DMA0CFGH;
        storedDescLow = DMA0CFGL;
        DMAARM |= (DMAARM_ABORT | DMAARM0);

        // Update descriptor with correct source.
        dmaDesc[0] = (unsigned long)&PM2_BUF >> 8;
        dmaDesc[1] = (unsigned long)&PM2_BUF;
        // Associate the descriptor with DMA channel 0 and arm the DMA channel
        DMA0CFGH = (unsigned long)&dmaDesc >> 8;
        DMA0CFGL = (unsigned long)&dmaDesc;
        DMAARM = DMAARM0;

        // NOTE! At this point, make sure all interrupts that will not be used to
        // wake from PM are disabled as described in the "Power Management Control"
        // chapter of the data sheet.

        // The following code is timing critical and should be done in the
        // order as shown here with no intervening code.

        // Align with positive 32 kHz clock edge as described in the
        // "Sleep Timer and Power Modes" chapter of the data sheet.
        temp = WORTIME0;
        while(temp == WORTIME0);
		
		/*

				if (sleepTimerInSeconds == 20)
				{
					setWOREVT1 = 0x02; // WOR_RES=2^10; LSB EVENT0; Use WOREVT0 = 0x80 and WOREVT1 = 0x02 (640dec) for EVENT0 value=> 20s timer
					setWOREVT0 = 0x80; // WOR_RES=2^10; MSB EVENT0; Use WOREVT0 = 0x80 and WOREVT1 = 0x02 (640dec) for EVENT0 value=> 20s timer
				}
				else if (sleepTimerInSeconds == 10)
				{
					setWOREVT1 = 0x01; // WOR_RES=2^10; LSB EVENT0; Use WOREVT0 = 0x40 and WOREVT1 = 0x01 (320dec) for EVENT0 value=> 10s timer
					setWOREVT0 = 0x40; // WOR_RES=2^10; MSB EVENT0; Use WOREVT0 = 0x40 and WOREVT1 = 0x01 (320dec) for EVENT0 value=> 10s timer
				}
				else if (sleepTimerInSeconds == 5)
				{
					setWOREVT1 = 0x00; // WOR_RES=2^10; LSB EVENT0; Use WOREVT0 = 0xA0 and WOREVT1 = 0x00 (160dec) for EVENT0 value=> 5s timer
					setWOREVT0 = 0xA0; // WOR_RES=2^10; MSB EVENT0; Use WOREVT0 = 0xA0 and WOREVT1 = 0x00 (160dec) for EVENT0 value=> 5s timer
				}
				else
				{
					setWOREVT1 = 0x00; // WOR_RES=2^10; LSB EVENT0; Use WOREVT0 = 0xA0 and WOREVT1 = 0x00 (160dec) for EVENT0 value=> 5s timer
					setWOREVT0 = 0xA0; // WOR_RES=2^10; MSB EVENT0; Use WOREVT0 = 0xA0 and WOREVT1 = 0x00 (160dec) for EVENT0 value=> 5s timer
				}
		*/		

        // Set Sleep Timer Interval
        //WOREVT1 = EVENT0_HIGH;
        //WOREVT0 = EVENT0_LOW;
				
				WOREVT1 = 0xEE;
				WOREVT0 = 0xEE;

        // Make sure HS XOSC is powered down when entering PM{2 - 3} and that
        // the flash cache is disabled.
        MEMCTR |= MEMCTR_CACHD;
        //SLEEP = 0x06;
				SLEEP = (SLEEP & ~SLEEP_MODE) | SLEEP_MODE_PM2;


		
        // Enter power mode as described in chapter "Power Management Control"
        // in the data sheet. Make sure DMA channel 0 is triggered just before
        // setting [PCON.IDLE].
        NOP();
        NOP();
        NOP();
        if(SLEEP & SLEEP_MODE)
        {
            //asm("MOV 0xD7,#0x01");      // DMAREQ = 0x01;
						DMAREQ = DMAARM0; // 0x01;
            NOP();                 // Needed to perfectly align the DMA transfer.
            //asm("ORL 0x87,#0x01");      // PCON |= 0x01 -- Now in PM2;
						PCON |= PCON_IDLE; //0x01;
            NOP();                 // First call when awake
        }
        // End of timing critical code

        // Enable Flash Cache.
        MEMCTR &= ~MEMCTR_CACHD;

        // Update DMA channel 0 with original descriptor and arm channel if it was
        // in use before PM was entered.
        DMA0CFGH = storedDescHigh;
        DMA0CFGL = storedDescLow;
        DMAARM = DMAARM0;

        ///////////////////////////////////////////////////////////////////////
        /////////// CC111xFx/CC251xFx Errata Note Code section End ////////////
        ///////////////////////////////////////////////////////////////////////
				
        // Wake up from powermode. After waking up, the system clock source is HS RCOSC.
				
				// We therefore need to set the clock and speed to the HS XOSC in order
				// to be able to use the radio! This is done at the start of the loop.

        // Wait until HS RCOSC is stable
        while( !(SLEEP & SLEEP_HFRC_S) );

        // Set LS XOSC as the clock oscillator for the Sleep Timer (CLKCON.OSC32 = 0)
        CLKCON &= ~CLKCON_OSC32;
	
    }
		

}

