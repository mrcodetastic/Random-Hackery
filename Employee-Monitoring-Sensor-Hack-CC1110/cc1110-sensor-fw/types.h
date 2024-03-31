#ifndef HAL_TYOES
#define HAL_TYOES

/*******************************************************************************
* If building with a C++ compiler, make all of the definitions in this header
* have a C binding.
*******************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif



	
/*==== CONSTS ================================================================*/
#ifndef FALSE
   #define FALSE 0
#endif

#ifndef TRUE
   #define TRUE 1
#endif

#ifndef NULL
   #define NULL 0
#endif

#ifndef HIGH
   #define HIGH 1
#endif

#ifndef LOW
   #define LOW 0
#endif


/// Input Pins
#define PIN7     0x80
#define PIN6     0x40
#define PIN5     0x20
#define PIN4     0x10
#define PIN3     0x08
#define PIN2     0x04
#define PIN1     0x02
#define PIN0     0x01	


/*******************************************************************************
* TYPEDEFS
*/

/* Boolean */
typedef unsigned char       BOOL;
typedef unsigned char       bool;
/* Data */
typedef unsigned char       byte;
typedef unsigned short      WORD;
typedef unsigned long       DWORD;


/** An unsigned 8-bit integer.  The range of this data type is 0 to 255. **/
typedef unsigned char  uint8;
	
/** A signed 8-bit integer.  The range of this data type is -128 to 127. **/
typedef signed   char  int8;

/** An unsigned 16-bit integer.  The range of this data type is 0 to 65,535. **/
typedef unsigned short uint16;

/** A signed 16-bit integer.  The range of this data type is -32,768 to 32,767. **/
typedef signed   short int16;

/** An unsigned 32-bit integer.  The range of this data type is 0 to 4,294,967,295. **/
typedef unsigned long  uint32;

/** A signed 32-bit integer.  The range of this data type is -2,147,483,648 to 2,147,483,647. **/
typedef signed   long  int32;

typedef unsigned char BIT;	


#if defined (SDCC) || defined (__SDCC)
	#define xdata __xdata
#endif


/*==== MACROS ===============================================================*/

// Bit mask
#define BM( b )       ( 0x01 << ( b ))

#define HIBYTE(a)     (BYTE) ((WORD)(a) >> 8 )
#define LOBYTE(a)     (BYTE)  (WORD)(a)

#define SET_WORD(regH, regL, word) \
   do{                             \
      (regH) = HIBYTE( word );     \
      (regL) = LOBYTE( word );     \
   }while(0)

// Macro to read a word out
// Must not be used for all registers as e.g. Timer1 and Timers2 require that regL is read first
#define GET_WORD(regH, regL, word) \
   do{                             \
      word = (WORD)regH << 8;      \
      word |= regL;                \
   }while(0)

	 

/*******************************************************************************
* Mark the end of the C bindings section for C++ compilers.
*******************************************************************************/
#ifdef __cplusplus
}
#endif
#endif