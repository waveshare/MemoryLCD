#ifndef _MEMORY_LCD_LIB_H
#define _MEMORY_LCD_LIB_H

#include <bcm2835.h>

//GPIO config
#define LCD_SCS         23      // Use any pin except the dedicated SPI SS pins?
#define LCD_DISP        24     	// Use any non-specialised GPIO pin
#define LCD_EXTCOMIN    25 		// Use any non-specialised GPIO pin

#define GPIO_HI HIGH
#define GPIO_LO LOW

#define ECOM(x)		bcm2835_gpio_write(LCD_EXTCOMIN, x)
#define CS(x)		bcm2835_gpio_write(LCD_SCS, x)
#define DISP(x)		bcm2835_gpio_write(LCD_DISP, x)

// Memory LCD pixel dimensions - ALTER ACCORDING TO YOUR PARTICULAR LCD MODEL
#define LCDWIDTH		(144)
#define LCDHEIGHT		(168)

// Delay constants for LCD timing   // (Datasheet values)
#define PWRUP_DISP_DELAY		40	// (>30us)
#define PWRUP_EXTCOMIN_DELAY	40  // (>30us)
#define SCS_HIGH_DELAY			3   // (>3us)
#define SCS_LOW_DELAY			1   // (>1us)
#define INTERFRAME_DELAY		1   // (>1us)

extern void initMemoryLCD(void);

// Write data direct to display
extern void writeLineToDisplay(uint8_t lineNumber, uint8_t *line);
extern void writeMultipleLinesToDisplay(uint8_t lineNumber, uint8_t numLines, uint8_t *lines);
// Write data to line buffer
extern void writePixelToLineBuffer(unsigned int pixel, uint8_t isWhite);
extern void writeByteToLineBuffer(uint8_t byteNumber, uint8_t byteToWrite);
extern void copyByteWithinLineBuffer(uint8_t sourceByte, uint8_t destinationByte);
extern void setLineBufferBlack(void);
extern void setLineBufferWhite(void);
// write data from line buffer to display
extern void writeLineBufferToDisplay(uint8_t lineNumber);
extern void writeLineBufferToDisplayRepeatedly(uint8_t lineNumber, uint8_t numLines);
// Write data to frame buffer
extern void writePixelToFrameBuffer(uint16_t pixel, uint16_t lineNumber, uint8_t isWhite);
extern void writeByteToFrameBuffer(uint8_t byteNumber, uint8_t lineNumber, uint8_t byteToWrite);
extern void setFrameBufferBlack(void);
extern void setFrameBufferWhite(void);
extern void setFrameBufferWith(uint8_t data);
// write data from frame buffer to display
extern void writeFrameBufferToDisplay(void);
// clear functions
extern void clearLineBuffer(void);
extern void clearFrameBuffer(void);
extern void clearDisplay(void);
// turn display on/off
extern void turnOff(void);
extern void turnOn(void);
// return display parameters
extern unsigned int getDisplayWidth(void);
extern unsigned int getDisplayHeight(void);
extern void* hardToggleVCOM(void *arg);
extern void drawImage(uint8_t* picData);
#endif
