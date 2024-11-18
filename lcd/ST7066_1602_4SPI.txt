//===============================================================
//    LCM     	 :	WH1602B
//    Contraller :	RW1063 
//    Author     :	
//    History    :	20210317 Theo
//===============================================================
#include	<reg51.h>
#include 	<stdio.h>          // define I/O functions
#include 	<INTRINS.H>        // KEIL FUNCTION


//===============================================================
//====I/O Function Define========================================

sbit		RS      =P3^0;
sbit		CSB		=P3^7;
sbit        SCL     =P3^4;
sbit        SDA     =P3^1;

//===============================================================
//====Character Define===========================================
#define  	Cword	0x10
#define		RAM_ADDR_1		0x80
#define		RAM_ADDR_2		0xc0

//===============================================================
//====Output Character===========================================
								//0123456789ABCDEF
unsigned char code MSG1[Cword]  ="******LCM*******";
unsigned char code MSG2[Cword]  ="==++++ESD+++++==";
unsigned char code MSG3[Cword]  ="==-2011/04/21-==";
unsigned char code MSG4[Cword]  ="**+Time:10:30+**";


//===============================================================
//====Output CGRAM===============================================
unsigned char code CGRAM1[8] ={0x1F,0x11,0x11,0x11,0x11,0x11,0x11,0x1F};
unsigned char code CGRAM2[8] ={0x0A,0x15,0x0A,0x15,0x0A,0x15,0x0A,0x15};	


//===============================================================
//====Sub-function Defune========================================
void Delay(char m);
void Com_WR(char command);
void Dat_WR(unsigned char byte_data);
void String_WR(char count, char *MSG);
void CGRAM();
void Ini_RW1063();



//===============================================================
//====Sub-function Time Delay====================================

void Delay(char m)
{
	unsigned char i,j,k;
	
	for(j = 0;j<m;j++)
	{
		for(k = 0; k<200;k++)
		{
			for(i = 0; i<200;i++)
			{
				_nop_();
			}
		}
	}	
}

//===============================================================
//====Sub-function Write Command=================================

void Com_WR(unsigned char command)
{
	unsigned char bMask;
	
	CSB = 0;
	RS = 0;

	for(bMask = 0x80; bMask; bMask >>= 1)
	{
		SCL = 0;
		
		_nop_();
		_nop_();
		
		SDA = command & bMask;
		SCL = 1;
		
		_nop_();
		_nop_();
	}
	RS = 1;
	CSB = 1;
}

//===============================================================
//====Sub-function Write Date====================================

void Dat_WR(unsigned char byte_data)
{
	unsigned char bMask;
	
	CSB = 0;
	RS = 1;

	for(bMask = 0x80; bMask; bMask >>= 1)
	{
		SCL = 0;
		
		_nop_();
		_nop_();
		
		SDA = byte_data & bMask;
		SCL = 1;
		
		_nop_();
		_nop_();
	}
	RS = 1;
	CSB = 1;
}

//===============================================================
//====Sub-function Write Strings=================================

void String_WR(char count, char *MSG)
{
	char i;
	
	for(i = 0; i<count;i++)
	{
		Dat_WR(MSG[i]);
	}
}

//===============================================================
//====Sub-function CGRAM=========================================

void CGRAM()
{
	unsigned char i;
	
	Com_WR(0x40|0x00);    //SET CG_RAM ADDRESS 000000

	for(i = 0;i<8;i++)
	{
		Dat_WR(CGRAM1[i]);
	}

	Com_WR(0x40|0x08);    //SET CG_RAM ADDRESS 001000
	
	for(i = 0;i<8;i++)
	{
		Dat_WR(CGRAM2[i]);
	}
}

//===============================================================
//====Sub-function Initial RW1063================================

void Ini_RW1063()
{
	
	RS		= 0;
	CSB		= 0;
	SCL		= 1;
	SDA		= 1;
	
	Com_WR(0x38);		//Function Set
	Com_WR(0x08);		//Display off
	Com_WR(0x01);		//Display Clear
	Delay(1);
	Com_WR(0x06);		//Entry mode set
	Com_WR(0x0c);		//Display On
}


void main()
{
	unsigned char i,mode,CHAR_ADD,CHAR_ADD1,font;
	
	mode=0;
	CHAR_ADD=0x41;
	CHAR_ADD1=0x61;
	font=0xBA;
	
	Ini_RW1063();

	CGRAM();               

	while (1){
		
		switch(mode)
		{
			case 0:           //show full on
				
				Com_WR(RAM_ADDR_1);
				
				for(i = 0; i<20;i++)
				{
					Dat_WR(0xFF);
					Delay(3);
				}
				
				Com_WR(RAM_ADDR_2);
				
				for(i = 0; i<20;i++)
				{
					Dat_WR(0xFF);		
				}
				
				Delay(6);
				
				mode=1;
				
				break;
				
			case 1:             // show char Max
				
				Com_WR(RAM_ADDR_1);
				for(i = 0; i<20;i++)
				{
					Dat_WR(CHAR_ADD+i);
				}
				
				Com_WR(RAM_ADDR_2);
				
				for(i = 0; i<20;i++)
				{
					Dat_WR(CHAR_ADD+i);
				}
				
				Delay(6);
				
				mode=2;
				
				break;
				
			case 2 :
				
				Com_WR(0x34);								
				Com_WR(RAM_ADDR_1);
				
				for(i = 0; i<20;i++)
				{
					Dat_WR(CHAR_ADD+i);		   //double high font
				}
				
				Delay(6);
				
				mode=3;
				
				break;

			case 3:                //show char shift
				
				Com_WR(0x38);								
				Com_WR(RAM_ADDR_1);
				
				for(i = 0; i<20;i++)
				{
					Dat_WR(CHAR_ADD1+i);
				}
				
				Com_WR(RAM_ADDR_2);
				
				for(i = 0; i<20;i++)
				{
					Dat_WR(CHAR_ADD1+i);
				}
				
				Delay(6);
				
				mode=4;
				
				break;
				
			case 4:                   //shift function
				
				Com_WR(0x1C);
				Com_WR(RAM_ADDR_1);
				
				for(i = 0; i<20;i++)
				{
					Dat_WR(CHAR_ADD1+i);
				}
				Com_WR(RAM_ADDR_2);
				for(i = 0; i<20;i++)
				{
					Dat_WR(CHAR_ADD1+i);
				}
				Delay(5);	
				
				Com_WR(0x1C);
				Com_WR(RAM_ADDR_1);
				
				for(i = 0; i<20;i++)
				{
					Dat_WR(CHAR_ADD1+i);
				}
				Com_WR(RAM_ADDR_2);
				
				for(i = 0; i<20;i++)
				{
					Dat_WR(CHAR_ADD1+i);
				}
				Delay(5);	
				
				Com_WR(0x1C);
				Com_WR(RAM_ADDR_1);
				
				for(i = 0; i<20;i++)
				{
					Dat_WR(CHAR_ADD1+i);
				}
				
				Com_WR(RAM_ADDR_2);
				
				for(i = 0; i<20;i++)
				{
					Dat_WR(CHAR_ADD1+i);
				}
				
				Delay(5);	 

				Com_WR(0x18);
				Com_WR(RAM_ADDR_1);
				
				for(i = 0; i<20;i++)
				{
					Dat_WR(CHAR_ADD1+i);
				}
				
				Com_WR(RAM_ADDR_2);
				
				for(i = 0; i<20;i++)
				{
					Dat_WR(CHAR_ADD1+i);
				}
				
				Delay(5);
				
				Com_WR(0x18);
				Com_WR(RAM_ADDR_1);
				
				for(i = 0; i<20;i++)
				{
					Dat_WR(CHAR_ADD1+i);
				}
				
				Com_WR(RAM_ADDR_2);
				
				for(i = 0; i<20;i++)
				{
					Dat_WR(CHAR_ADD1+i);
				}
				Delay(5);
				
				Com_WR(0x18);
				Com_WR(RAM_ADDR_1);
				for(i = 0; i<20;i++)
				{
					Dat_WR(CHAR_ADD1+i);
				}
				
				Com_WR(RAM_ADDR_2);
				
				for(i = 0; i<20;i++)
				{
					Dat_WR(CHAR_ADD1+i);
				}
					
				Delay(6);
								
				mode=5;
				
				break;	   

			case 5:               //show SQUARE
				
				Com_WR(RAM_ADDR_1);
				
				for(i = 0; i<20;i++)
				{
					Dat_WR(0x00);
				}
				
				Com_WR(RAM_ADDR_2);
				
				for(i = 0; i<20;i++)
				{
					Dat_WR(0x00);
				}
				
				Delay(6);
				
				mode=6;
				
				break;
				
			case 6:                    //show CROSS DOT   
				
				Com_WR(RAM_ADDR_1);
				for(i = 0; i<20;i++)
				{
					Dat_WR(0x01);
				}
				
				Com_WR(RAM_ADDR_2);
				for(i = 0; i<20;i++)
				{
					Dat_WR(0x01);
				}
				
				Delay(6);
				
				mode=7;
				
				break;
				
			case 7:               // SHOW FONT
				
				Com_WR(RAM_ADDR_1);
				for(i = 0; i<20;i++)
				{
					Dat_WR(font);
				}
				
				Com_WR(RAM_ADDR_2);
				for(i = 0; i<20;i++)
				{
					Dat_WR(font);
				}
				
				Delay(6);
				
				mode=8;
				
				break;
				
			case 8:
			
				Com_WR(RAM_ADDR_1);
				String_WR(Cword,MSG1);
				Com_WR(RAM_ADDR_2);
				String_WR(Cword,MSG2);
					
				Delay(15);
				
				Com_WR(RAM_ADDR_1);
				String_WR(Cword,MSG3);
				Com_WR(RAM_ADDR_2);
				String_WR(Cword,MSG4);
					
				Delay(15);
				
				mode = 0;

				break;
					
		}				
	}
	
	Delay(30);
}
 