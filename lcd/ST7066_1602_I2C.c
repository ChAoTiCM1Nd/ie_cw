//===============================================================
//    LCM     	 :	WH1602B
//    Contraller :	RW1063 
//    Author     :	
//    History    :	20210304 Theo
//===============================================================

#include	<reg51.h>
#include 	<stdio.h>          // define I/O functions
#include 	<INTRINS.H>        // KEIL FUNCTION

//===============================================================
//====I2C ADDRESS Define=========================================

#define		I2CSA0	2
#define		I2CSA1	4


//===============================================================
//====I/O Function Define========================================
	
sbit		RS      =P3^0;
sbit		CSB		=P1^5;
sbit		SA0		=P1^0;
sbit		SA1		=P1^1;
sbit        SCL     =P1^7;
sbit        SDA     =P1^6;


//===============================================================
//====Character Define===========================================

#define  	Cword	0x10
#define		one		0x80						
#define		two		0xc0


//===============================================================
//====Output Character===========================================

unsigned char code MSG1[Cword]  ="   25 C         ";
unsigned char code MSG2[Cword]  ="Have nice day!! ";
		   
//===============================================================
//====Output CGRAM===============================================

unsigned char code CGRAM1[]={
0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,	//00H	全黑
0x1f,0x11,0x11,0x11,0x11,0x11,0x11,0x1f,	//01H	黑框
0x0a,0x15,0x0a,0x15,0x0a,0x15,0x0a,0x15,	//02H	雪花
0x0a,0x15,0x0a,0x15,0x0a,0x15,0x0a,0x15		//03H	雪花
};


//===============================================================
//====Sub-function Defune========================================

void START_CON();
void STOP_CON();
void Ack_Check();
void CGRAM();
void Write_control(unsigned char);
void Writeadds();
void WRITE_CODE(unsigned char,unsigned char);
void Write_data(unsigned char) ;
void delay(char);
void Initial_RW1063();

//===============================================================
//====Sub-function I2C Protocol==================================

void START_CON()
{
	SDA = 1; 
	SCL = 1; 

	SDA = 0;
	SCL = 0;
}

void STOP_CON()
{
	SCL = 1;
	SDA = 1;
}

void Ack_Check()
{
	SDA = 1;
	SCL = 1;  
	
	do{
		SDA = 0;
		SCL = 0; 
	}while(SDA);
}


//===============================================================
//====Sub-function Time Daley====================================

void delay(char m)
{
	unsigned char i,j,k;
	
	for(j = 0;j<m;j++)
	{
		for(k = 0; k<200;k++)
		{
			for(i = 0; i<200;i++)
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
	
	WRITE_CODE(0x00,0x40);    //SET CG_RAM ADDRESS 000000
	_nop_();
	_nop_();
	
	for(i = 0;i<32;i++){
		WRITE_CODE(0x40,CGRAM1[i]);
		_nop_();_nop_();
	}
}


//===============================================================
//====Sub-function I2C Write (Address + Command + Data)==========

void WRITE_CODE(unsigned char I2C_CONTROL, unsigned char I2C_data)
{
	START_CON();
	Writeadds() ;
	Write_control(I2C_CONTROL);
	Write_data(I2C_data) ;
	STOP_CON();
}

void Writeadds()
{
	unsigned char adds = 0x78;// = 0x7C;
	unsigned char count=0;

	adds = adds + I2CSA0 + I2CSA1;
	for(count=0;count<8;count++){
		if(adds & 0x80) 
			SDA=1;
		else 
			SDA=0;
			
		adds<<=1; 
		SCL=1; 
		SCL=0; 
	}
	Ack_Check();
}

void Write_control(unsigned char C_CONTROL)
{
	unsigned char count=0;

	for(count=0;count<8;count++){
		if(C_CONTROL&0x80) 
			SDA=1;
		else 
			SDA=0;
			
		C_CONTROL<<=1; 
		SCL=1; 
		SCL=0; 
	}
	Ack_Check();
}

void Write_data(unsigned char data1)
{
	unsigned char count=0;

	for(count=0;count<8;count++){
		if(data1&0x80)
			SDA=1;
		else 
			SDA=0;
			
		data1 <<= 1; 
		SCL=1; 
		SCL=0; 
	}
	Ack_Check();
}

//===============================================================
//====Sub-function Initial RW1063================================

void Initial_RW1063()
{
	RS		= 0;
	CSB		= 0;
	SA0		= I2CSA0;
	SA1		= I2CSA1;
	SCL     = 1;
	SDA     = 1;

	/****************************************************
	Function Set
	8'b 0_0_1_DL N_F_X_X
	DL: 8-bit/4-bit
	N: Lines
	L: 5x11/5x8	
	IF using IIC and 4-SPI interface、DL bit must be setting to “1”
	****************************************************/
	WRITE_CODE(0x00,0x38);
	
	CGRAM();

	/****************************************************
	Display OFF (1_D_C_B)
	****************************************************/
	WRITE_CODE(0x00,0x08);
	
	/****************************************************
	Entry Mode
	****************************************************/
	WRITE_CODE(0x00,0x06);
	
	/****************************************************
	Clear Display (0.76ms)
	****************************************************/
	WRITE_CODE(0x00,0x01);
	
	delay(1);
	
	/****************************************************
	Display ON
	****************************************************/
	WRITE_CODE(0x00,0x0c);
}


//===============================================================
//====Main Function==============================================

void main()
{
	unsigned char i,mode,CHAR_ADD,CHAR_ADD1,font;
	
	Initial_RW1063();
	CGRAM();
	
	mode=0;
	CHAR_ADD=0x41;
	CHAR_ADD1=0x61;
	font=0xA7;

	while(1){	
		switch(mode){
			case 0:           //show full on
				
				WRITE_CODE(0x00,one);
				
				for(i = 0; i<20;i++)
				{
					WRITE_CODE(0x40,0x26); //0x26 : &
					delay(10);
				}
				
				WRITE_CODE(0x00,two);
				
				for(i = 0; i<20;i++)
					WRITE_CODE(0x40,0x02);
				
				delay(10);
				mode=1;
				break;
				
			case 1:             // show char Max
				
				WRITE_CODE(0x00,one);
				for(i = 0; i<20;i++)
				WRITE_CODE(0x40,CHAR_ADD+i);
				
				WRITE_CODE(0x00,two);
				for(i = 0; i<20;i++)
				WRITE_CODE(0x40,CHAR_ADD+i);
				//stp_sc();
				delay(10);
				mode=2;
				break;
				
			case 2 :
				WRITE_CODE(0x00,0x34);
				
				WRITE_CODE(0x00,one);
				for(i = 0; i<20;i++)
				WRITE_CODE(0x40,CHAR_ADD+i);		   //double high font
				//stp_sc();
				delay(10);
				mode=3;
				break;

			case 3:                //show char shift
				WRITE_CODE(0x00,0x38);
				
				WRITE_CODE(0x00,one);
				for(i = 0; i<20;i++)
				WRITE_CODE(0x40,CHAR_ADD1+i);
				
				WRITE_CODE(0x00,two);
				for(i = 0; i<20;i++)
				WRITE_CODE(0x40,CHAR_ADD1+i);
				//stp_sc();
				delay(10);
				mode=4;
				break;
				
			case 4:                   //shift function
				WRITE_CODE(0x00,0x1c);
				
				WRITE_CODE(0x00,one);
				for(i = 0; i<20;i++)
					WRITE_CODE(0x40,CHAR_ADD1+i);
				
				WRITE_CODE(0x00,two);
				for(i = 0; i<20;i++)
					WRITE_CODE(0x40,CHAR_ADD1+i);
				delay(5);	
				
				WRITE_CODE(0x00,0x1c);
				
				WRITE_CODE(0x00,one);
				for(i = 0; i<20;i++)
					WRITE_CODE(0x40,CHAR_ADD1+i);
				
				WRITE_CODE(0x00,two);
				for(i = 0; i<20;i++)
					WRITE_CODE(0x40,CHAR_ADD1+i);
				delay(5);	
				
				WRITE_CODE(0x00,0x1c);
				WRITE_CODE(0x00,one);
				for(i = 0; i<20;i++)
					WRITE_CODE(0x40,CHAR_ADD1+i);
					
				WRITE_CODE(0x00,two);
				for(i = 0; i<20;i++)
					WRITE_CODE(0x40,CHAR_ADD1+i);
				delay(5);	 

				WRITE_CODE(0x00,0x18);
				WRITE_CODE(0x00,one);
				for(i = 0; i<20;i++)
					WRITE_CODE(0x40,CHAR_ADD1+i);
					
				WRITE_CODE(0x00,two);
				for(i = 0; i<20;i++)
					WRITE_CODE(0x40,CHAR_ADD1+i); 
				delay(5);
				
				WRITE_CODE(0x00,0x18);
				
				WRITE_CODE(0x00,one);
				for(i = 0; i<20;i++)
					WRITE_CODE(0x40,CHAR_ADD1+i);
					
				WRITE_CODE(0x00,two);
				for(i = 0; i<20;i++)
					WRITE_CODE(0x40,CHAR_ADD1+i); 
				delay(5);
				
				WRITE_CODE(0x00,0x18);
				WRITE_CODE(0x00,one);
				for(i = 0; i<20;i++)
					WRITE_CODE(0x40,CHAR_ADD1+i);
					
				WRITE_CODE(0x00,two);
				for(i = 0; i<20;i++)
					WRITE_CODE(0x40,CHAR_ADD1+i); 
					
				delay(5);
				//stp_sc();
				delay(10);
				mode=5;
				break;	   

			case 5:               //show SQUARE
				
				WRITE_CODE(0x00,one);
				for(i = 0; i<20;i++)
					WRITE_CODE(0x40,0x00);
					
				WRITE_CODE(0x00,two);
				for(i = 0; i<20;i++)
					WRITE_CODE(0x40,0x00);
				//stp_sc();
				delay(10);
				mode=6;
				break;
				
			case 6:                    //show CROSS DOT   

				WRITE_CODE(0x00,one);
				for(i = 0; i<20;i++)
					WRITE_CODE(0x40,0x01);
				WRITE_CODE(0x00,two);
				
				for(i = 0; i<20;i++)
					WRITE_CODE(0x40,0x01);
				//stp_sc();
				delay(10);
				mode=7;
				break;
				
			case 7:               // SHOW FONT
				
				WRITE_CODE(0x00,one);
				for(i = 0; i<20;i++)
					WRITE_CODE(0x40,font);
					
				WRITE_CODE(0x00,two);
				for(i = 0; i<20;i++)
					WRITE_CODE(0x40,font);
				//stp_sc();
				delay(10);
				mode=0;
				break;
		}				
	}	 
}




 