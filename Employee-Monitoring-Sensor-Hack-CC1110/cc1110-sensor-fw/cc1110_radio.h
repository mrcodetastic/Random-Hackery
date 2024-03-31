#ifndef RADIO
#define RADIO

#include "cc1110.h"
#include "ioCCxx10_bitdef.h"
#include "types.h"
#include <stdio.h>
#include <string.h>

/***********************************************************************************
* RADIO CONSTANTS
*/

#define DEVICE_NUMBER			1
#define DESTINATION_ADDR	0x00 	// What device do we send this too, or is it broadcast?
#define MAX_PACKET_SIZE		61	
#define MAX_PAYLOAD_SIZE 	MAX_PACKET_SIZE-4	


/*==== CONSTS ================================================================*/
// https://github.com/hayesey/cc1110/blob/master/radio/radio_isr/radio.c

//INTERRUPT(rftxrx_isr, RFTXRX_VECTOR)
static uint8 packet_index;

// Page 196 of cc1110-cc11110
static unsigned char xdata packet_header[] = {DESTINATION_ADDR, MAX_PAYLOAD_SIZE, 1, 1}; // destination, size, stream num of packets, seq number
static unsigned char xdata packet[MAX_PACKET_SIZE] = {0};

/*==== ISR ================================================================*/

INTERRUPT(rftxrx_isr, RFTXRX_VECTOR)
{
  // flash the LED on P0_0
  P1_0 ^= 1; // yellow led on
	
  switch (MARCSTATE) {
    case MARC_STATE_RX:
      // receive byte
      packet[packet_index++] = RFD;
      break;
    case MARC_STATE_TX:
      // transmit byte
      RFD = packet[packet_index++];
      break;
  } 
	
	
  P1_0 ^= 1; // yellow led off	
}

/*******************************************************************************
* If building with a C++ compiler, make all of the definitions in this header
* have a C binding.
*******************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif
	
	
void send_packet() {

  // use timer 3 to delay tx to allow time to switch from tx to rx
	
  T3CTL=0xDC;
  T3OVFIF=0; 
  while (!T3OVFIF);
  T3CTL=0;

	

  packet_index = 0;
	
  RFST = RFST_STX;
  while (MARCSTATE != MARC_STATE_TX);
	
  // tx happens here
  while (MARCSTATE != MARC_STATE_IDLE);
	
  RFIF=0;
	
	
	// Reset
	packet_index = 0;
}

	
void radio_start() 
{  
	
	 //   P1_0 ^= 1; // on
	

	  // Configure the radio interrupt flags
		RFIF = 0; // RX Interrupt Flag
		RFTXRXIF = 0;	
	
		// radio init
		RFST=RFST_SIDLE; // enter idle state
	
	
    /* NOTE: The register settings are hard-coded for the predefined set of data
     * rates and frequencies. To enable other data rates or frequencies these
     * register settings should be adjusted accordingly (use SmartRF(R) Studio).
     */
/*
		# Address Config = No address check 
		# Base Frequency = 868.299866 
		# CRC Enable = true 
		# Carrier Frequency = 871.499084 
		# Channel Number = 16 
		# Channel Spacing = 199.951172 
		# Data Rate = 249.939 
		# Deviation = 126.953125 
		# Device Address = 0 
		# Manchester Enable = false 
		# Modulated = true 
		# Modulation Format = GFSK 
		# PA Ramping = false 
		# Packet Length = 61 
		# Packet Length Mode = Fixed packet length mode. Length configured in PKTLEN register 
		# Preamble Count = 4 
		# RX Filter BW = 541.666667 
		# Sync Word Qualifier Mode = 30/32 sync word bits detected 
		# TX Power = 0 
		# Whitening = true 
		# ---------------------------------------------------
		# Setting for uVision Project CC1110
		# ---------------------------------------------------
*/
		PKTLEN    = MAX_PACKET_SIZE;  // Packet Length  - 61 fixed
		PKTCTRL0  = 0x44;  // Packet Automation Control - fixed packet size with whitening
		CHANNR    = 0x10;  // Channel Number  - 16
		FSCTRL1   = 0x0C;  // Frequency Synthesizer Control 
		FREQ2     = 0x21;  // Frequency Control Word, High Byte 
		FREQ1     = 0x65;  // Frequency Control Word, Middle Byte 
		FREQ0     = 0x6A;  // Frequency Control Word, Low Byte 
		MDMCFG4   = 0x2D;  // Modem configuration 
		MDMCFG3   = 0x3B;  // Modem Configuration 
		MDMCFG2   = 0x13;  // Modem Configuration 
		DEVIATN   = 0x62;  // Modem Deviation Setting 
		MCSM0     = 0x30;  // Main Radio Control State Machine Configuration 		-- go idle after sending packet
		MCSM0     = 0x18;  // Main Radio Control State Machine Configuration 
		FOCCFG    = 0x1D;  // Frequency Offset Compensation Configuration 
		BSCFG     = 0x1C;  // Bit Synchronization Configuration 
		AGCCTRL2  = 0xC7;  // AGC Control 
		AGCCTRL1  = 0x00;  // AGC Control 
		AGCCTRL0  = 0xB0;  // AGC Control 
		FREND1    = 0xB6;  // Front End RX Configuration 
		FSCAL3    = 0xEA;  // Frequency Synthesizer Calibration 
		FSCAL2    = 0x2A;  // Frequency Synthesizer Calibration 
		FSCAL1    = 0x00;  // Frequency Synthesizer Calibration 
		FSCAL0    = 0x1F;  // Frequency Synthesizer Calibration 
		TEST1     = 0x31;  // Various Test Settings 
		TEST0     = 0x09;  // Various Test Settings 
		PA_TABLE0 = 0x50;  // PA Power Setting 0 
		
		
		// Packet 0ing
		packet_index = 0;

		//enable interrupts.
		RFTXRXIF=0;
		RFTXRXIE=1;		
		
		RFST=RFST_SIDLE;
		while(MARCSTATE!=MARC_STATE_IDLE);		
		
	 //   P1_0 ^= 1; // off		
	
    return;
}

	
	
	
	
/*******************************************************************************
* Mark the end of the C bindings section for C++ compilers.
*******************************************************************************/
#ifdef __cplusplus
}
#endif
#endif