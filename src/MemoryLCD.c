#include "MemoryLCD.h"
#include <bcm2835.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "Debug.h"

uint8_t commandByte = 0x80;
uint8_t vcomByte = 0x40;
uint8_t clearByte = 0x20;
uint8_t paddingByte = 0x00;

uint8_t lineBuffer[LCDWIDTH / 8];
unsigned char frameBuffer[LCDWIDTH * LCDHEIGHT / 8];
uint8_t reverseByte(uint8_t b);

void spi_writenb(uint8_t *data, uint32_t length)
{
  bcm2835_spi_writenb((const char *)data, length);
}

void initMemoryLCD(void)
{
  if (!bcm2835_init())
  {
	printf("bcm2835 init failed  !!! \r\n");
    return;
  }
  else
  {
	printf("bcm2835 init success !!! \r\n");
  }

  bcm2835_gpio_fsel(LCD_SCS, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_fsel(LCD_DISP, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_fsel(LCD_EXTCOMIN, BCM2835_GPIO_FSEL_OUTP);

  // Introduce delay to allow MemoryLCD to reach 5V
  // (probably redundant here as Pi's boot is much longer than Arduino's power-on time)
  delayMicroseconds(800); // minimum 750us

  // setup separate thread to continuously output the EXTCOMIN signal for as long as the parent runs.
  // NB: this leaves the Memory LCD vulnerable if an image is left displayed after the program stops.
  pthread_t threadId;
  //if(enablePWM) {
  if (pthread_create(&threadId, NULL, &hardToggleVCOM, (void *)LCD_EXTCOMIN))
  {
	printf("Error creating EXTCOMIN thread  !!! \r\n");
  }
  else
  {
	printf("PWM thread started successfully  !!! \r\n");
  }

  // SETUP SPI
  // Datasheet says SPI clock must have <1MHz frequency (BCM2835_SPI_CLOCK_DIVIDER_256)
  // but it may work up to 4MHz (BCM2835_SPI_CLOCK_DIVIDER_128, BCM2835_SPI_CLOCK_DIVIDER_64)
  /*
   * The Raspberry Pi GPIO pins reserved for SPI once bcm2835_spi_begin() is called are:
   * P1-19 (MOSI)
   * P1-21 (MISO)
   * P1-23 (CLK)
   * P1-24 (CE0)
   * P1-26 (CE1)
   */
  bcm2835_spi_begin();
  // set MSB here - setting to LSB elsewhere doesn't work. So I'm manually reversing lineAddress bit order instead.
  bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
  bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_128); // this is the 2 MHz setting
  bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
  bcm2835_spi_chipSelect(BCM2835_SPI_CS_NONE);

  // Not sure if I can use the built-in bcm2835 Chip Select functions as the docs suggest it only
  // affects bcm2835_spi_transfer() calls so I'm setting it to inactive and setting up my own CS pin
  // as I want to use spi_writenb() to send data over SPI instead.

  // Set pin modes
  CS(GPIO_LO);
  DISP(GPIO_LO);
  ECOM(GPIO_LO);

  // Memory LCD startup sequence with recommended timings
  DISP(GPIO_HI);
  bcm2835_delayMicroseconds(PWRUP_DISP_DELAY);

  ECOM(GPIO_LO);
  bcm2835_delayMicroseconds(PWRUP_EXTCOMIN_DELAY);

  clearLineBuffer();
}

void writeLineToDisplay(uint8_t lineNumber, uint8_t *line)
{
  writeMultipleLinesToDisplay(lineNumber, 1, line);
}

void writeMultipleLinesToDisplay(uint8_t lineNumber, uint8_t numLines, uint8_t *lines)
{
  // this implementation writes multiple lines that are CONSECUTIVE (although they don't
  // have to be, as an address is given for every line, not just the first in the sequence)
  // data for all lines should be stored in a single array
  uint8_t *linePtr = lines;

  CS(GPIO_HI);
  bcm2835_delayMicroseconds(SCS_HIGH_DELAY);
  spi_writenb(&commandByte, 1);
  for (char x = 0; x < numLines; x++)
  {
    uint8_t reversedLineNumber = reverseByte(lineNumber);
    spi_writenb(&reversedLineNumber, 1);
    // Transfers a whole line of data at once
    // pointer arithmetic assumes an array of lines - TODO: change this?
    spi_writenb(linePtr + (LCDWIDTH / 8) * x, LCDWIDTH / 8);
    spi_writenb(&paddingByte, 1);
    lineNumber++;
  }
  spi_writenb(&paddingByte, 1); // trailing paddings
  bcm2835_delayMicroseconds(SCS_LOW_DELAY);

  CS(GPIO_LO);
  bcm2835_delayMicroseconds(INTERFRAME_DELAY); // can I delete this delay?
}

void writePixelToLineBuffer(unsigned int pixel, uint8_t isWhite)
{
  // pixel location expected in the fn args follows the scheme defined in the datasheet.
  // NB: the datasheet defines pixel addresses starting from 1, NOT 0
  if ((pixel <= LCDWIDTH) && (pixel != 0))
  {
    pixel = pixel - 1;
    if (isWhite)
      lineBuffer[pixel / 8] |= (1 << (7 - pixel % 8));
    else
      lineBuffer[pixel / 8] &= ~(1 << (7 - pixel % 8));
  }
}

void writeByteToLineBuffer(uint8_t byteNumber, uint8_t byteToWrite)
{
  // char location expected in the fn args has been extrapolated from the pixel location
  // format (see above), so chars go from 1 to LCDWIDTH/8, not from 0
  if (byteNumber <= LCDWIDTH / 8 && byteNumber != 0)
  {
    byteNumber -= 1;
    lineBuffer[byteNumber] = byteToWrite;
  }
}

void copyByteWithinLineBuffer(uint8_t sourceByte, uint8_t destinationByte)
{
  if (sourceByte <= LCDWIDTH / 8 && destinationByte <= LCDWIDTH / 8)
  {
    sourceByte -= 1;
    destinationByte -= 1;
    lineBuffer[destinationByte] = lineBuffer[sourceByte];
  }
}

void setLineBufferBlack(void)
{
  for (uint16_t i = 0; i < LCDWIDTH / 8; i++)
  {
    lineBuffer[i] = 0x00;
  }
}

void setLineBufferWhite(void)
{
  for (uint16_t i = 0; i < LCDWIDTH / 8; i++)
  {
    lineBuffer[i] = 0xFF;
  }
}

void writeLineBufferToDisplay(uint8_t lineNumber)
{
  writeMultipleLinesToDisplay(lineNumber, 1, lineBuffer);
}

void writeLineBufferToDisplayRepeatedly(uint8_t lineNumber, uint8_t numLines)
{
  writeMultipleLinesToDisplay(lineNumber, numLines, lineBuffer);
}

void writePixelToFrameBuffer(uint16_t pixel, uint16_t lineNumber, uint8_t isWhite)
{
  // pixel location expected in the fn args follows the scheme defined in the datasheet.
  // NB: the datasheet defines pixel addresses starting from 1, NOT 0
  if ((pixel <= LCDWIDTH) && (pixel != 0) && (lineNumber <= LCDHEIGHT) & (lineNumber != 0))
  {
    pixel -= 1;
    lineNumber -= 1;
    if (isWhite)
      frameBuffer[(lineNumber * LCDWIDTH / 8) + (pixel / 8)] |= (1 << (7 - pixel % 8));
    else
      frameBuffer[(lineNumber * LCDWIDTH / 8) + (pixel / 8)] &= ~(1 << (7 - pixel % 8));
  }
}

void writeByteToFrameBuffer(uint8_t byteNumber, uint8_t lineNumber, uint8_t byteToWrite)
{
  // char location expected in the fn args has been extrapolated from the pixel location
  // format (see above), so chars go from 1 to LCDWIDTH/8, not from 0
  if ((byteNumber <= LCDWIDTH / 8) && (byteNumber != 0) && (lineNumber <= LCDHEIGHT) & (lineNumber != 0))
  {
    byteNumber -= 1;
    lineNumber -= 1;
    frameBuffer[(lineNumber * LCDWIDTH / 8) + byteNumber] = byteToWrite;
  }
}

void setFrameBufferBlack()
{
  for (uint16_t i = 0; i < LCDWIDTH * LCDHEIGHT / 8; i++)
  {
    frameBuffer[i] = 0x00;
  }
}

void setFrameBufferWhite()
{
  for (uint16_t i = 0; i < LCDWIDTH * LCDHEIGHT / 8; i++)
  {
    frameBuffer[i] = 0xFF;
  }
}

void setFrameBufferWith(uint8_t data)
{
  for (uint16_t i = 0; i < LCDWIDTH * LCDHEIGHT / 8; i++)
  {
    frameBuffer[i] = data;
  }
}

void drawImage(uint8_t *picData)
{
  for (uint16_t i = 0; i < LCDHEIGHT * LCDWIDTH / 8; i++)
  {
    frameBuffer[i] = picData[i];
  }
}

void writeFrameBufferToDisplay()
{
  writeMultipleLinesToDisplay(1, LCDHEIGHT, frameBuffer);
}

void clearLineBuffer()
{
  setLineBufferWhite();
}

void clearFrameBuffer()
{
  setFrameBufferWhite();
}

void clearDisplay()
{
  CS(GPIO_HI);
  bcm2835_delayMicroseconds(SCS_HIGH_DELAY);

  spi_writenb(&clearByte, 1);
  spi_writenb(&paddingByte, 1);
  bcm2835_delayMicroseconds(SCS_LOW_DELAY);

  CS(GPIO_LO);
  bcm2835_delayMicroseconds(INTERFRAME_DELAY);
}

// won't work if DISP pin is not used
void turnOff()
{
  DISP(GPIO_HI);
}

// won't work if DISP pin is not used
void turnOn()
{
  DISP(GPIO_LO);
}

unsigned int getDisplayWidth()
{
  return LCDWIDTH;
}

unsigned int getDisplayHeight()
{
  return LCDHEIGHT;
}

void *hardToggleVCOM(void *arg)
{
  //char extcomin = (char)arg;
  char extcomin = *((char *)(&arg));
  //cout << "Value of extcomin in hardToggleVCOM is " << (unsigned int)extcomin << "\n" << endl;
  printf("extcomin start\r\n");
  // intended to be run as a separate thread. Do not execute in main loop!
  while (1)
  {
    bcm2835_delay(250);
    bcm2835_gpio_write(extcomin, HIGH);
    bcm2835_delay(250);
    bcm2835_gpio_write(extcomin, LOW);

    //cout << "toggle ExtComIn\n" <<endl;
    printf("PWM thread started successfully\r\n");
  }
  pthread_exit(NULL);
}

uint8_t reverseByte(uint8_t b)
{
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
  return b;
}
