/*  --------------------------------------------------------------------
    FILE:           lcdi2c.c
    PROJECT:        Pinguino - http://www.pinguino.cc/
    PURPOSE:        Driving a LCD display through i2c PCF8574 I/O expander
    PROGRAMER:      Regis Blanchot <rblanchot@gmail.com>
    FIRST RELEASE:  29 Jul. 2008
    LAST RELEASE:   15 Jun. 2011
    --------------------------------------------------------------------
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    ------------------------------------------------------------------*/

/*  --------------------------------------------------------------------
    LCD 2x16 (GDM1602A with build-in Samsung KS0066/S6A0069)
    --------------------------------------------------------------------

    01 - VSS (GRND)
    02 - VDD (+5V)
    03 - Vo (R = 1KOhm à la masse)
    04 - RS
    05 - RW (can be connected to GND so that RW = 0 = write)
    06 - EN
    07 a 10 - D0 to D3 connected to GND (4-bit mode).
    11 a 16 - D4 to D7 connected to PCF8574's pins (see below)
    15 - LED+ R330
    16 - LED- GND or Backlight pin in PCF8574
    --------------------------------------------------------------------------*/

/*  --------------------------------------------------------------------
    PCF8574P
    --------------------------------------------------------------------

    +5V		A0		-|o |-		VDD		+5V
    +5V		A1		-|	|-		SDA		pull-up 1K8 au +5V
    +5V		A2		-|	|-		SCL 	pull-up 1K8 au +5V
    LCD_BL	P0		-|	|-		INT
    LCD_RS	P1		-|	|-		P7		LCD_D7
    LCD_RW	P2		-|	|-		P6		LCD_D6
    LCD_EN	P3		-|	|-		P5		LCD_D5
    GRND	VSS		-|	|-		P4		LCD_D4

    SYMBOL 	PIN		DESCRIPTION					NB
    A0		1		address input 0				adress = 0 1 0 0 A2 A1 A0 0
    A1		2		address input 1				A0, A1 et A2 connected to +5V
    A2		3		address input 2				then address is 01001110 = 0x4E
    P0		4		quasi-bidirectional I/O 0	LCD_BL
    P1		5		quasi-bidirectional I/O 1	LCD_RS
    P2		6		quasi-bidirectional I/O 2	LCD_RW
    P3		7		quasi-bidirectional I/O 3	LCD_EN
    VSS		8		supply ground
    P4		9		quasi-bidirectional I/O 4	LCD_D4
    P5		10		quasi-bidirectional I/O 5	LCD_D5
    P6		11		quasi-bidirectional I/O 6	LCD_D6
    P7		12		quasi-bidirectional I/O 7	LCD_D7
    INT		13		interrupt output (active LOW)
    SCL		14		serial clock line			uC_SCL
    SDA		15		serial data line			uC_SDA
    VDD		16		supply voltage
    --------------------------------------------------------------------------*/

#ifndef __LCDI2C_C
#define __LCDI2C_C

#include <typedef.h>
#include <lcdi2c.h>
//#include <pcf8574.h>
#include <delay.c>
#include <i2c.c>

// Printf
#ifdef LCDI2CPRINTF
    #include <printFormated.c>
#endif

// PrintFloat
#if defined(LCDI2CPRINTFLOAT)
    #include <printFloat.c>
#endif

// PrintNumber
#if defined(LCDI2CPRINTNUMBER) || defined(LCDI2CPRINTFLOAT)
    #include <printNumber.c>
#endif

#define I2C_write(c)		I2C_writechar(I2C1, c)

/*  --------------------------------------------------------------------
    Défintion des caractères spéciaux
    --------------------------------------------------------------------
    â, à, ç, é, î, ô, ù, ê, è, ë, ï, û, €
    usage :
    1/ réservation de l'emplacement 0 (max. 7) pour la lettre "é" :
       lcdi2c_newchar(car3, 0);
    2/ écriture du nouveau caractère sur le LCD : lcdi2c_write(0);
    ------------------------------------------------------------------*/
#ifndef __32MX220F032D__
    const u8 car0[8]={
        0b00000100,      //â
        0b00001010,
        0b00001110,
        0b00000001,
        0b00001111,
        0b00010001,
        0b00001111,
        0b00000000
    };
    const u8 car1[8]={
        0b00000100,     //à
        0b00000010,
        0b00001110,
        0b00000001,
        0b00001111,
        0b00010001,
        0b00001111,
        0b00000000
    };
    const u8 car2[8]={
        0b00001110,     //ç
        0b00010000,
        0b00010000,
        0b00010001,
        0b00001110,
        0b00000100,
        0b00001100,
        0b00000000
    };
    const u8 car3[8]={
        0b00000100,     //é
        0b00001000,
        0b00001110,
        0b00010001,
        0b00011111,
        0b00010000,
        0b00001110,
        0b00000000
    };
    const u8 car4[8]={
        0b00000100,     //è
        0b00000010,
        0b00001110,
        0b00010001,
        0b00011111,
        0b00010000,
        0b00001110,
        0b00000000
    };
    const u8 car5[8]={
        0b00000100,     //ê
        0b00001010,
        0b00001110,
        0b00010001,
        0b00011111,
        0b00010000,
        0b00001110,
        0b00000000
    };
    const u8 car6[8]={
        0b00001010,     //ë
        0b00000000,
        0b00001110,
        0b00010001,
        0b00011111,
        0b00010000,
        0b00001110,
        0b00000000
    };
    const u8 car7[8]={
        0b00000111,     //€
        0b00001000,
        0b00011110,
        0b00001000,
        0b00011110,
        0b00001000,
        0b00000111,
        0b00000000
    };
    const u8 car8[8]={
        0b00000100,     //î
        0b00001010,
        0b00001100,
        0b00000100,
        0b00000100,
        0b00000100,
        0b00001110,
        0b00000000
    };
    const u8 car9[8]={
        0b00001010,     //ï
        0b00000000,
        0b00001100,
        0b00000100,
        0b00000100,
        0b00000100,
        0b00001110,
        0b00000000
    };
    const u8 car10[8]={
        0b00000100,     //ô
        0b00001010,
        0b00001110,
        0b00010001,
        0b00010001,
        0b00010001,
        0b00001110,
        0b00000000
    };
    const u8 car11[8]={
        0b00000100,     //ù
        0b00000010,
        0b00010001,
        0b00010001,
        0b00010001,
        0b00010011,
        0b00001101,
        0b00000000
    };
    const u8 car12[8]={
        0b00000100,     //û
        0b00001010,
        0b00010001,
        0b00010001,
        0b00010001,
        0b00010011,
        0b00001101,
        0b00000000
    };
#endif /* __32MX220F032D__ */

/*  --------------------------------------------------------------------
    global variables
    ------------------------------------------------------------------*/

    u8 gLCDWIDTH;                           // from 0 to 15 = 16
    u8 gLCDHEIGHT;                          // from 0 to 1 = 2
    u8 gBacklight = 0;                      // backlight status
    u8 PCF8574_address;
    u8 PCF8574_data;
    u8 posd4_7;
    u8 pos_en;
    u8 pos_rw;
    u8 pos_rs;
    u8 pos_bl;

/*  --------------------------------------------------------------------
    Ecriture d'un quartet (mode 4 bits) dans le LCD
    --------------------------------------------------------------------
    Envoie d'un quartet vers les pins :
    - D4 a D7 du LCD
    - P4 a P7 du PCF8574
    NB : quartet est en fait un 8 bits dont seuls les 4 bits de poids fort nous interessent
    @param quartet = 4 bits a envoyer au LCD
    @param mode = LCD Command (LCD_CMD) or Data (LCD_DATA) mode
    ------------------------------------------------------------------*/

static void lcdi2c_send4(u8 quartet, u8 mode)
{
    PCF8574_data = quartet;                    // x  x  x  x  0  0  0    0
    if(mode)
        PCF8574_data |= (mode << pos_rs);      // x  x  x  x  0  0  0/1  0
    if(gBacklight)
        PCF8574_data |= (gBacklight << pos_bl);// x  x  x  x  0  0  0/1  0

    /// ---------- LCD Enable Cycle

    I2C_start(I2C1);                            // send start condition

    //I2C_writechar(PCF8574_address | I2C_WRITE);
    I2C_write((PCF8574_address << 1) | I2C_WRITE);

    PCF8574_data |= (1 << pos_en);
    I2C_write(PCF8574_data);
    // E Pulse Width > 300ns

    PCF8574_data &= ~(1 << pos_en);
    I2C_write(PCF8574_data);
    // E Enable Cycle > (300 + 200) = 500ns

    I2C_stop(I2C1);                             // send stop confition
}

/*  --------------------------------------------------------------------
    Ecriture d'un octet dans le LCD en mode 4 bits
    --------------------------------------------------------------------
    Les données sont écrites en envoyant séquentiellement :
    1/ les quatre bits de poids fort
    2/ les quatre bits de poids faible
    NB : les poids sont stockes dans les quatre bits de poids fort
    qui correspondent aux pins D4 a D7 du LCD ou du PCF8574
    @param octet = octet a envoyer au LCD
    @param mode = LCD Command (LCD_CMD) or Data (LCD_DATA) mode
    ------------------------------------------------------------------*/

static void lcdi2c_send8(u8 octet, u8 mode)
{
    if (posd4_7 == 0)
    {
        lcdi2c_send4((octet>>4), mode);     // send upper 4 bits
        lcdi2c_send4(octet & 0x0F, mode);   // send lower 4 bits
    }
    else
    {
        lcdi2c_send4(octet & 0xF0, mode);   // send upper 4 bits
        lcdi2c_send4((octet <<4), mode);    // send lower 4 bits
    }
    //Delayus(46);                          // Wait for instruction excution time (more than 46us)
}

/*  --------------------------------------------------------------------
    backlight
    NB : PCF8574 is logical inverted so :
    0 = ON
    1 = OFF
    ------------------------------------------------------------------*/

#if defined(LCDI2CBACKLIGHT) || defined (LCDI2CNOBACKLIGHT)
void lcdi2c_blight(u8 status)
{
    gBacklight = status;
    PCF8574_data |= (gBacklight << pos_bl);
    I2C_start(I2C1);
    I2C_write((PCF8574_address << 1) | I2C_WRITE);
    I2C_write(PCF8574_data);
    I2C_stop(I2C1);
}
#endif

/*  --------------------------------------------------------------------
    Positionne le curseur sur le LCD
    from (0,0) to (15,1)
    ------------------------------------------------------------------*/

#if defined(LCDI2CSETCURSOR)
void lcdi2c_setCursor(u8 col, u8 line)
{
    int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };

    if (line > gLCDHEIGHT)
        line = gLCDHEIGHT;// - 1;           // we count rows starting w/0
        
    if (col > gLCDWIDTH)
        col = gLCDWIDTH;  // - 1;           // we count rows starting w/0
        
    lcdi2c_send8(LCD_SETDDRAMADDR | (col + row_offsets[line]), LCD_CMD);
}
#endif

/*  --------------------------------------------------------------------
    Efface une ligne
    ------------------------------------------------------------------*/

#if defined(LCDI2CCLEARLINE)
void lcdi2c_clearLine(u8 line)
{
    u8 i;

    lcdi2c_setCursor(0, line);
    for (i = 0; i <= gLCDWIDTH; i++)
        lcdi2c_write(SPACE);                // display spaces
}
#endif

/*  --------------------------------------------------------------------
    Affiche un caractere ASCII a la position courante du curseur
    c = code ASCII du caractere
    ------------------------------------------------------------------*/

void lcdi2c_printChar(u8 c)
{
    if (c < 32) c = 32;                     // replace ESC char with space
    lcdi2c_send8(c, LCD_DATA);
}

/*  --------------------------------------------------------------------
    print
    ------------------------------------------------------------------*/

#if defined(LCDI2CPRINT)       || defined(LCDI2CPRINTLN)    || \
    defined(LCDI2CPRINTNUMBER) || defined(LCDI2CPRINTFLOAT) || \
    defined(LCDI2CPRINTCENTER)

void lcdi2c_print(const u8 *string)
{
    while (*string)
        lcdi2c_printChar(*string++);
}

#endif

/*  --------------------------------------------------------------------
    println : useless on a LCD
    ------------------------------------------------------------------*/

#if 0
#if defined(LCDI2CPRINTLN)
void lcdi2c_println(const u8 *string)
{
    lcdi2c_print(string);
    lcdi2c_print((u8*)"\n\r");
}
#endif
#endif

/*  --------------------------------------------------------------------
    printCenter : centers text in line
    ------------------------------------------------------------------*/

#if defined(LCDI2CPRINTCENTER)
void lcdi2c_printCenter(const u8 *string)
{
    u8 len=0, nbspace;
    const u8 *p = string;

    while (*p++) len++;
    nbspace = (gLCDWIDTH + 1 - len) / 2;
    
    // write spaces before
    while(nbspace--)
        lcdi2c_send8((u8)SPACE, LCD_DATA);

    // write string
    lcdi2c_print(string);
}
#endif

/*  --------------------------------------------------------------------
    printNumber
    ------------------------------------------------------------------*/

#if defined(LCDI2CPRINTNUMBER) || defined(LCDI2CPRINTFLOAT)
void lcdi2c_printNumber(s32 value, u8 base)
{
    printNumber(lcdi2c_printChar, value, base);
}
#endif

/*  --------------------------------------------------------------------
    printFloat
    ------------------------------------------------------------------*/

#if defined(LCDI2CPRINTFLOAT)
void lcdi2c_printFloat(float number, u8 digits)
{
    printFloat(lcdi2c_printChar, number, digits);
}
#endif

/*  --------------------------------------------------------------------
    printf
    ------------------------------------------------------------------*/

#if defined(LCDI2CPRINTF)
void lcdi2c_printf(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    pprintf(lcdi2c_printChar, fmt, args);
    va_end(args);
}
#endif

/*  --------------------------------------------------------------------
    Définit un caractère personnalisé de 8x8 points.
    --------------------------------------------------------------------
    d'après Nabil Al-HOSSRI <http://nalhossri.free.fr>
    Le LCD utilisé admet au maximum 8 caractères spéciaux.
    char_code : code du caractère à définir (0 <= char_code <= 7)
    lcdi2c_newchar(car3, 0); Définit le caractère 'é' à l'adresse 0 de la mémoire CG RAM
    lcdi2c_newchar(car8, 1); Définit le caractère 'è' à l'adresse 1.
    ------------------------------------------------------------------*/

//#if defined(LCDI2CNEWCHAR)
void lcdi2c_newchar(const u8 *c, u8 char_code)
{
    u8 i, a;

    // les caractères sont logés dans le CGRAM du LCD à partir de l'adresse 0x40.
    a = (char_code << 3) | LCD_ADRESS_CGRAM;
    for(i=0; i<8; i++)
    {
        lcdi2c_send8(a, LCD_CMD);
        lcdi2c_send8(c[i], LCD_DATA);
        a++;
    };
}
//#endif

/*  --------------------------------------------------------------------
    Définition de 8 nouveaux caractères
    ------------------------------------------------------------------*/

#ifndef __32MX220F032D__
void lcdi2c_newpattern()
{
    lcdi2c_newchar(car0,  ACIRC);		// â
    lcdi2c_newchar(car1,  AGRAVE);		// à

    lcdi2c_newchar(car2,  CCEDIL);		// ç

    lcdi2c_newchar(car3,  EACUTE);		// é
    lcdi2c_newchar(car4,  EGRAVE);		// è
    lcdi2c_newchar(car5,  ECIRC);		// ê
    lcdi2c_newchar(car6,  ETREMA);		// ë
    lcdi2c_newchar(car7,  EURO);		// €

    //lcdi2c_newchar(car8,  ICIRC);		// î
    //lcdi2c_newchar(car9, ITREMA);		// ï

    //lcdi2c_newchar(car10,  OCIRC);	// ô

    //lcdi2c_newchar(car11,  UGRAVE);	// ù
    //lcdi2c_newchar(car12, UCIRC);		// û
}
#endif

/*  --------------------------------------------------------------------
    Initialisation du LCD
    --------------------------------------------------------------------
    This function must be called before any other function.
    No need to wait between 2 commands because i2c bus is quite slow.
    cf. Microchip AN587 Interfacing PICmicro® MCUs to an LCD Module
    --------------------------------------------------------------------
    pcf8574 adress format is [0 1 0 0 A2 A1 A0 0]
    --------------------------------------------------------------------
    usage : lcdi2c.init(16, 2, 0x27, 4, 2, 1, 0, 3);
    ------------------------------------------------------------------*/

void lcdi2c_init(u8 numcol, u8 numline, u8 i2c_address, u8 d4_7, u8 en, u8 rw, u8 rs, u8 bl)
{
    u8 cmd03 = 0x03, cmd02 = 0x02;
    gLCDWIDTH  = numcol - 1;
    gLCDHEIGHT = numline - 1;
    PCF8574_address = i2c_address;
    PCF8574_data = 0;
    posd4_7 = d4_7;
    pos_en = en;
    pos_rw = rw;
    pos_rs = rs;
    pos_bl = bl;

    if(posd4_7 != 0)
    {
        cmd03=0x30;
        cmd02=0x20;
    }

    //I2C_init(I2C1, I2C_MASTER_MODE, I2C_100KHZ);
    I2C_init(I2C1, I2C_MASTER_MODE, I2C_400KHZ);
    //I2C_init(I2C1, I2C_MASTER_MODE, I2C_1MHZ);

    Delayms(15);                                // Wait more than 15 ms after VDD rises to 4.5V
    lcdi2c_send4(cmd03, LCD_CMD);               // 0x30 - Mode 8 bits
    Delayms(5);                                 // Wait for more than 4.1 ms
    lcdi2c_send4(cmd03, LCD_CMD);               // 0x30 - Mode 8 bits
    //Delayus(100);                             // Wait more than 100 μs
    lcdi2c_send4(cmd03, LCD_CMD);               // 0x30 - Mode 8 bits
    //Delayus(100);                             // Wait more than 100 μs
    lcdi2c_send4(cmd02, LCD_CMD);               // 0x20 - Mode 4 bits
    lcdi2c_send8(LCD_SYSTEM_SET_4BITS, LCD_CMD);// 0x28 - Mode 4 bits - 2 Lignes - 5x8
    //Delayus(4);                               // Wait more than 40 ns
    lcdi2c_send8(LCD_DISPLAY_ON, LCD_CMD);      // 0x0C - Display ON + Cursor OFF + Blinking OFF
    //Delayus(4);                               // Wait more than 40 ns
    lcdi2c_send8(LCD_DISPLAY_CLEAR, LCD_CMD);   // 0x01 - Efface l'affichage + init. DDRAM
    Delayms(2);                                 // Execution time > 1.64ms
    lcdi2c_send8(LCD_ENTRY_MODE_SET, LCD_CMD);  // 0x06 - Increment + Display not shifted (Déplacement automatique du curseur)
    //Delayus(4);                               // Wait more than 40 ns
    #ifndef __32MX220F032D__
    lcdi2c_newpattern();                        // Set new characters
    #endif
}

#endif
