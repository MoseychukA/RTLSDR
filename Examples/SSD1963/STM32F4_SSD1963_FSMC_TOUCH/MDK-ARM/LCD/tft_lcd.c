#include "stm32f4xx.h"

#include "tft_lcd.h"
#include "font.h"				   // ����� 
#include "correct_ANSI.h" // ��������� ��������� ���� �������� ����� ���. � ����.
//#include "delay.h"         // ��������� ��� �-��� ������������� �������
//#include "img.h"
#include "stdlib.h"


char array [100]; 
/*******************************************************************/
//�������
void LCD_SendCommand(uint16_t com)
{
	CMD_ADR = com;
}
/*******************************************************************/
// ������..
void LCD_SendData(uint16_t data)
{   
	
    DAT_ADR= data; 	
}


void LCD_Clear( uint16_t color)
{	     
	LCD_SetCursorPosition(0, 0,  LCD_WIDTH - 1, LCD_HEIGHT - 1);
	  uint32_t n = LCD_PIXEL_COUNT;
		while (n) 
			{
				n--;
        DAT_ADR= color; 	
	  	}     
} 


 

// ********* �������� ���� � ������ LCD ************// 
// ������ � ��� ���� ����� �������� ���������� ����, ���������� ��� �� �� ����������
// x1 - ������ ���������, �1 - ������ �����������, x2 - ����� ���������, �2 - ����� �����������
void LCD_SetCursorPosition(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) 
{
	LCD_SendCommand(ILI9341_COLUMN_ADDR); // ��� ��������� ��.������
	LCD_SendData (y1 >> 8);                // "�����" ��� ����� "�������" � ���������� �� ������
	LCD_SendData(y1 & 0xFF);
	LCD_SendData (y2 >> 8);
	LCD_SendData(y2 & 0xFF);


	LCD_SendCommand(ILI9341_PAGE_ADDR); // ��� ��������� ��.������
  LCD_SendData(x1 >> 8);
	LCD_SendData(x1 & 0xFF);
	LCD_SendData(x2 >> 8);
	LCD_SendData(x2 & 0xFF);

  LCD_SendCommand(ILI9341_GRAM); // ���������� � ������ LCD
}




void WriteString(unsigned char x0,unsigned int y0,  char *s,unsigned int color)//, unsigned int back_color) ��������, ���� ���� �������� ���������� ���� ����
{

		
  	unsigned char z, y, nb, he, wi, d, j, q, st1, en1,y2;
	  unsigned int x, i, k;
	  y2 = y0;            // �������� ���������� ������� ��� ������ �������� ������� ������
		d = FONT_info [0];  // ������ �������, ����, ������� �� ����� ������
		wi=d*8;             // ����������� ������ ������� , ��� 
	  he = FONT_info [1]; // ������ �������, �����, ������� �� ����� ������
		nb = (he*d);        //��������� ���-�� ���� �� ���� ������

  for(z = 0;s[z]!='\0';z++) // ���������� ��� ������� � ������, ���� �� ������ �� ��������
    {
		if(s [z] < 128) 										// ���� ������ ���������, ��..
    {i = (s [z]-32)*nb;}                //������������ ������ �� ������ 32 �������
    else if (s [z] > 191)  							// ���� ������ �������, ��..
		{i =(s [z]-32-(FONT_info [2]))*nb;} //������� ���-�� ����., ����� 127-� � 192-� �� ANSI		(��.���� ������ � �����)
		else 																// ���� ����� ���. � ����, �.�. ����� 127 � 192, ��	
		{ 																	// ���� �������������� � ���� correct_ANSI.c � ������������ � ������ ������������ 
			i = (corr_ANSI ((unsigned char*)s, z))*nb;				// , �.�. ������ ������� �� �������� �� ������ ������ �������� - ���������
		}
																     // ������ �������� �������� ������ �������� �������, �.�. ��������� ������ ������� ����� � ������ �� �������
																     // ����� �������� ������� ������� ����, ������, �� ��������� ����� (PS. ������ ������� �� ��������)
		x = i;                         // �������� ����� ������� ����� ������� � ������� (��������� ����, � �������� ���������� ������ � �������)
		st1=0;											   // ������ �������, �� ����������� ����������� �������
		en1=0;											   // -*-*-
		for (q = 0; q < he; q++)       // ���������� (��������) "������" �������
		{				
			for(j = 0, k = 0; j < d; j++)// ���������� ��� ����� ������, ����� ���� ������� ��� ����� ������� ������, 
			{ 													 // PS. "�" - ������� ���� � ���� ������, ����� ���������� ����� ����� �������
				 	y = 8;      						 // ������� ��� ������ ����� ������
          while (y--) 						 // ���������� � ��������� ��� �� �����
            {
							k++;					       // ���������� ������� ���� � ������							
		      		if (((FONT[x]&(1<<y)) && (st1 == 0))||((FONT[x]&(1<<y)) && (k < st1))) {st1 = k;} // ���� ����� ���� ����� ����� ����� ��� =1							
	      			if  ((FONT[x]&(1<<y)) && (en1 < k))  	{en1 = k;}							// ���� ����� ���� ����� ����� ������ ��� =1
		      	}
						x++; 									// ��������� ���� ����� �������
			}	
		}	
		
		if (st1 > 0) {st1--;} 				// ������� ���������, ����� ������� �� "���������"
		if (en1 < 2){en1 = wi/3;}				// ���� ������ ������, �� ��� ������, ������ ��� ������ 1/3 �� ������ �������
		else if (en1 < wi){en1 = (en1 - st1 + indent);} // ����������� �������� ������ � ���������� ������ (��. font.h), ���� ���������� ���������� ����� ����.

		j=0;      // �������� "����������-���������" ��� ��� ������ ���� � ������
		k = nb+i; // �������� ����� ���������� (� �������) �����  ����� �������
		q = en1;  // �������� ������ �������, ��� , en1 �������� "��������������" ������ ����� �������, ���� ��������� "������"
		
		
		if ((y0+en1) > LCD_HEIGHT) {y0=y2; x0++;}  // ���� ��������� ������ �� ���������, �� ��������� �� ��������� ������ (LCD_HEIGHT - ��.������)
																							 // ���� �� ����� �������, �� ���������� ������� ������� "�����" ����� � ����� ����� ����� ������
		LCD_SetCursorPosition(x0*he, y0, (x0*he)+(he-1), (y0)+(en1-1)); // �������� ���� � ������ LCD, � - ���������, � - �����������
		y0=y0+en1; 																											// ��������� �0, ��� � ������ ����� ������ ���������� �������
		
		
     for(x = i; x < k; x++) // �������� (� �������) �� ������� ��� ����� �������� �������
    {
					if (j++ == 0) {y = 10-st1;} // ���� ��� ������ ���� ������ (j=0), �� �������� ������ ���� ������ ������, �� ���������� ���������� 2 pt
					else {y = 8;}               // ������ ��� �� ������ ���� ������
       while (y--)                    // ������� ���� ������, �.�. ��������� ��� ���� (?=0 ��� ?=1) ����� �����
        {									
       if((FONT[x]&(1<<y))!=0)        // ��� =1 ? ��� =0 ?
       { LCD_SendData(color);}     // ���� �� (=1) ����� ����  ������� (16-bit SPI)
			 		 
       else
				 { LCD_SendData (BLACK);}     // ���� ��� (=0) ����� ����	����  (16-bit SPI) //BLACK <- back_color);} ��������, ���� ���� �������� ���������� ���� ����

				if(!--q)                      // ������� ������� �������������� ��� ������ ������, ���� ��� ��� (������ ����� ������)
					{
						if (j != d){x = x+(d-j);} // ��.. ��������, ����� ����������� ������ ������ ��������� ������ ��� �� ����, ���� ���, �� ��������� �������� 
						                          // (������ ����� � �������) (��������: ���������� ����� = 3���, � �� 1 ������ �������� 3 ����� = 24���, 2 ���� "������")
					  y = 0; j = 0; q = en1;    // �� � ����� ������ �������� ������� ���, ���� ������ � ������� ������ ������
					} 			
        }
     }  
    }		
}

void LCD_DrawPoint(uint16_t ysta, uint16_t xsta, uint16_t color)
{
	LCD_SetCursorPosition(xsta, ysta, xsta, ysta);
	LCD_SendData(color); 
}



void LCD_DrawHLine(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t color)
{
	unsigned int y;

	LCD_SetCursorPosition(xsta, ysta, xend, ysta);  		
	y =  ((xend-xsta)+1);

	while(y--)
	{
		DAT_ADR= color; 	
	}
}



/*********************************************************************************************************
** Functoin name:       LCD_DrawVLine
*********************************************************************************************************/
void LCD_DrawVLine(uint16_t xsta, uint16_t ysta, uint16_t yend, uint16_t color)
{
	unsigned int y;

	LCD_SetCursorPosition(xsta, ysta, xsta, yend);  		
	y =  ((yend-ysta)+1);
	while(y--)
	{
		DAT_ADR= color; 	
	}
}



void LCD_DrawLine(uint16_t ysta, uint16_t xsta, uint16_t yend, uint16_t xend, uint16_t color)
{
    uint16_t x, y, t;	 
	if((xsta==xend)&&(ysta==yend))
		LCD_DrawPoint(xsta, ysta, color);
	else if(xsta==xend)
	{
		LCD_DrawVLine(xsta,ysta,yend,color);
	}
	else if(ysta==yend)
	{	
		LCD_DrawHLine(xsta,ysta,xend,color);
	}
	else{ 
		if(abs(yend-ysta)>abs(xend-xsta))
		{
			if(ysta>yend) 
			{
				t=ysta;
				ysta=yend;
				yend=t; 
				t=xsta;
				xsta=xend;
				xend=t; 
			}
			for(y=ysta;y<yend;y++)
			{
				x = (uint32_t)(y-ysta)*(xend-xsta)/(yend-ysta)+xsta;
				LCD_DrawPoint(x, y, color);  
			}
		}
		else
		{
			if(xsta>xend)
			{
				t=ysta;
				ysta=yend;
				yend=t;
				t=xsta;
				xsta=xend;
				xend=t;
			}   
			for(x=xsta;x<=xend;x++)
			{
				y = (uint32_t)(x-xsta)*(yend-ysta)/(xend-xsta)+ysta;
				LCD_DrawPoint(x, y, color); 
			}
		}
	} 
}

void bright_PWM_ssd1963(uint8_t bright)
{
LCD_SendCommand(0xBE);    // PWM configuration 
LCD_SendData(0x08);     // set PWM signal frequency to 170Hz when PLL frequency is 100MHz 
LCD_SendData(bright);     // PWM duty cycle  
	LCD_SendData(0x01);     //
}




//******************************************************************************
//***       ������������� �������
//******************************************************************************
void LCD_ini (void)
	{
	//1. Power up the system platform and assert the RESET# signal (�L� state) for a minimum of 100us to reset the controller. 
		TFT_RST_SET
    HAL_Delay (100);
    TFT_RST_RESET
    HAL_Delay (120);
    TFT_RST_SET
    HAL_Delay (120);
	/***************************
	2. Configure SSD1961�s PLL frequency 
VCO = Input clock x (M + 1) 
PLL frequency  = VCO / (N + 1) 
* Note : 
1.  250MHz < VCO < 800MHz 
PLL frequency < 110MHz 
2.  For a 10MHz input clock to obtain 100MHz PLL frequency, user cannot program M = 19 and N = 1.  The 
closet setting in this situation is setting M=29 and N=2, where 10 x 30 / 3 = 100MHz. 
3.  Before PLL is locked, SSD1961/2/3 is operating at input clock frequency (e.g. 10MHz), registers 
programming cannot be set faster than half of the input clock frequency (5M words/s in this example). 
Example to program SSD1961 with M = 29, N = 2, VCO = 10M x 30 = 300 MHz, PLL frequency = 300M / 3 = 100 
MHz 
	******************************/
		

	LCD_SendCommand(0xE2);  //��������� �������
LCD_SendData(0x1D);  // ��������(M=29) 
LCD_SendData(0x02);  //���������(N=2) 
LCD_SendData(0xFF);  //���/���� ������ � ���.

//3. Turn on the PLL 
LCD_SendCommand(0xE0);  
LCD_SendData(0x01); 
 HAL_Delay (120); // Wait for 100us to let the PLL stable and read the PLL lock status bit. 
LCD_SendCommand(0xE0); 
//READ COMMAND �0xE4);   (Bit 2 = 1 if PLL locked) 
LCD_SendData(0x03); // 5. Switch the clock source to PLL 
  HAL_Delay (120);
LCD_SendCommand(0x01); //6. Software Reset
HAL_Delay (120);
/*************
Dot clock Freq = PLL Freq x (LCDC_FPR + 1) / 2(� 20 �������)
For example,  22MHz = 100MHz * (LCDC_FPR+1) / 2 (� 20 �������)
LCDC_FPR = 230685 = 0x3851D 
********************/
LCD_SendCommand(0xE6);  // 7. Configure the dot clock frequency // ��������� ������� �������������
LCD_SendData(0x03); 
LCD_SendData(0x85);  
LCD_SendData(0x1D);  

//8. Configure the LCD panel  
//a. Set the panel size to 480 x 800 and polarity of LSHIFT, LLINE and LFRAME to active low 
LCD_SendCommand(0xB0); 
LCD_SendData(0x0C);   // 18bit panel, disable dithering, LSHIFT: Data latch in rising edge, LLINE and LFRAME: active low 
LCD_SendData(0x00);     // TFT type 
LCD_SendData(0x03);     // Horizontal Width:  480 - 1 = 0x031F 
LCD_SendData(0x1F);  
LCD_SendData(0x01);     // Vertical Width :  800 -1 = 0x01DF
LCD_SendData(0xDF);  
LCD_SendData(0x00);     // 000 = ����� RGB

//b. Set the horizontal period 
LCD_SendCommand(0xB4);    // Horizontal Display Period  
LCD_SendData(0x03);    // HT: horizontal total period (display + non-display) � 1 = 520-1 =  519 =0x0207
LCD_SendData(0xA0);      
LCD_SendData(0x00);    // HPS: Horizontal Sync Pulse Start Position = Horizontal Pulse Width + Horizontal Back Porch = 16 = 0x10 
LCD_SendData(0x2E);    
LCD_SendData(0x30);     // HPW: Horizontal Sync Pulse Width - 1=8-1=7 
LCD_SendData(0x00);    // LPS: Horizontal Display Period Start Position = 0x0000 
LCD_SendData(0x0F); 
LCD_SendData(0x00);    // LPSPP: Horizontal Sync Pulse Subpixel Start Position(for serial TFT interface).  Dummy value for TFT interface. 

//c. Set the vertical period 
LCD_SendCommand(0xB6);    // Vertical Display Period  
LCD_SendData(0x02);     // VT: Vertical Total (display + non-display) Period � 1  =647=0x287 
LCD_SendData(0x0D);    
LCD_SendData(0x00);     // VPS: Vertical Sync Pulse Start Position  =     Vertical Pulse Width + Vertical Back Porch = 2+2=4  
LCD_SendData(0x10);    
LCD_SendData(0x10);     //VPW: Vertical Sync Pulse Width � 1 =1 
LCD_SendData(0x08);     //FPS: Vertical Display Period Start Position = 0 
LCD_SendData(0x00);  
/****
9. Set the back light control PWM clock frequency  // ��������� ��� ���������
PWM signal frequency = PLL clock / (256 * (PWMF[7:0] + 1)) / 256 
********/
LCD_SendCommand(0xBE);    // PWM configuration 
LCD_SendData(0x08);     // set PWM signal frequency to 170Hz when PLL frequency is 100MHz 
LCD_SendData(0xFF);     // PWM duty cycle  (50%) 
LCD_SendData(0x01);     // 0x09 = enable DBC, 0x01 = disable DBC  //��������


LCD_SendCommand(0x36);     // set address_mode
LCD_SendData(0x02);        // ��������� ����������, ���������, RGB/BGR � ��.

//13. Setup the MCU interface for 16-bit data write (565 RGB)
LCD_SendCommand(0xF0);     // mcu interface config 
LCD_SendData(0x03);     // 16 bit interface (565)

//10. Turn on the display 
LCD_SendCommand(0x29);     // display on 
}
