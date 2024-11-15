//===============================================================
//    LCM     	 :	WH1602B
//    Contraller :	ST7066 
//    Author     :	
//    History    :	20210304 Theo
//===============================================================

#include	<reg51.h>
#include 	<stdio.h>          // define I/O functions
#include 	<INTRINS.H>        // KEIL FUNCTION

//===============================================================
//====I/O Function Define========================================
	
#define		Data_BUS P1

sbit		busy    =P1^7;
sbit		RS      =P3^0;
sbit	 	RW      =P3^7;
sbit		Enable  =P3^4;

//===============================================================
//====Character Define===========================================

#define  	Cword	0x10
#define		one		0x80						
#define		two		0xc0


//===============================================================		   
//====Global Vari Define=========================================

char bdata  flag;
unsigned char x, y, count; 
sbit busy_f  = flag^0;


//===============================================================
//====Output Character===========================================

unsigned char code MSG1[Cword]  ="ABCDEFGHIJKLMNOP";
unsigned char code MSG2[Cword]  ="QRSTUVWXYZ012345"; 
unsigned char code MSG3[Cword]  =">>>>>>>>>>>>>>>>"; 
unsigned char code MSG4[Cword]  ="<<<<<<<<<<<<<<<<"; 


//===============================================================
//====Output CGRAM===============================================

unsigned char code CGRAM1[8] ={0x02,0x05,0x02,0x00,0x00,0x00,0x00,0x00};
unsigned char code CGRAM2[8] ={0x04,0x04,0x04,0x04,0x04,0x15,0x0e,0x04,};  // ?
unsigned char code CGRAM3[8] ={0x1f,0x11,0x11,0x11,0x11,0x11,0x11,0x1f,};  // ?
unsigned char code CGRAM4[8] ={0x0a,0x15,0x0a,0x15,0x0a,0x15,0x0a,0x15,};  // ?


//===============================================================
//====Sub-function Defune========================================

void delay(char);
void CGRAM();
void CheckBusy();
void WriteIns(char);
void WriteData(char);
void WriteString(char,char *);
void Initial_KS0066();


//===============================================================
//====Sub-function Check Busy Flag===============================

void CheckBusy()
{
	Data_BUS = 0xff; //???high??low????,??????high.
	
	RS = 0;
	RW = 1;  
	
	do
	{
	  Enable = 1;
	  busy_f = busy;
	  Enable = 0;
	 }while(busy_f);
}


//===============================================================
//====Sub-function Write Instruction=============================

void WriteIns(char instruction)
{
	RS = 0;
	RW = 0;

	Data_BUS = instruction;

	Enable = 1;
	_nop_();
	Enable = 0;

	delay(1);	
}

//===============================================================
//====Sub-function Write Data====================================

void WriteData(char data1)
{
	RS = 1;
	RW = 0;

	Data_BUS = data1;

	Enable = 1;
	_nop_();
	Enable = 0;

	delay(1);
	CheckBusy(); 
}

//===============================================================
//====Sub-function Write String==================================

void WriteString(count,MSG)
char count;
char *MSG;
{
	char i;
	
	for(i = 0; i < count; i++)
	{
		WriteData(MSG[i]);
	}	
}

//===============================================================
//====Sub-function Time Daley====================================

void delay(char m)
{
	unsigned char i, j, k;
	
	for(j = 0;j < m; j++)
 	{
	 	for(k = 0; k < 200; k++)
	 	{
			for(i = 0; i < 200; i++)
			{
			}
		}
 	}	
}

//===============================================================
//====Sub-function CGRAM=========================================

void CGRAM()
{
	unsigned char i;

	WriteIns(0x40);
	
	for(i = 0;i<8;i++)
	{
	   	WriteData(CGRAM3[i]);
	}	
	
}

//===============================================================
//====Sub-function Initial_KS0066================================

void Initial_KS0066()
{
	/****************************************************
	Function Set
	8'b 0_0_1_DL N_F_X_X
	DL: 8-bit/4-bit
	N: Lines
	L: 5x11/5x8		
	****************************************************/

	WriteIns(0x38);
	WriteIns(0x38);
	WriteIns(0x38);
	WriteIns(0x38);

	/****************************************************
	Entry Mode
	Set cursor move direction and display shift
	8'b 0000_ 0_1_ID_S
	S: Sfift of entire display
	I/D : Incerament/Decreament of DDR address	
	****************************************************/
	
	WriteIns(0x06);
	
	/****************************************************
	Clear Display / 1.52ms
	****************************************************/
	WriteIns(0x01);
	
	/****************************************************
	Display ON/OFF
	8'b 0000_1DCB
	D: entire display on
	C: Cursor on
	B: Blink cusor position
	****************************************************/
	WriteIns(0x0e);
	
	/****************************************************
	Entry Mode
	****************************************************/
	WriteIns(0x06);
}

//===============================================================
//====Main Function==============================================

main()
{
	unsigned char i;

	Initial_KS0066();	
	CGRAM();	
	
	while(1)
	{
		/****************************************************
		Set DDR Address in AC counter AC[6:0]
		****************************************************/
		WriteIns(0xa0);
		WriteString(16, MSG3);
		
		WriteIns(0x80);
		WriteString(16, MSG1);
		
		WriteIns(0x8f);
		WriteString(16, MSG2);
		
		WriteIns(0xb0);
		WriteString(16, MSG4);
		
		delay(20);


		/****************************************************
		Shift Display
		****************************************************/
		
		for(i <= 0; i <= 999; i++)
		{
			WriteIns(0x18);
			if(i == 50)
			{
				WriteIns(0x01);
			}
			delay(3);
		}
	
	}	
}


 