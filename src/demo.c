#include "demo.h"
#include "MemoryLCD.h"
#include "math.h"
#include "ugui.h"

unsigned int lcdWidth;
unsigned int lcdHeight;
char numRepetitions = 4;

extern const unsigned char gImage_pic[3024];

void sleep(uint32_t ms)
{
	bcm2835_delay(ms);
}

void warpWritePixelToFrameBuffer(UG_S16 x, UG_S16 y, UG_U16 color)
{
	writePixelToFrameBuffer(x + 1, y + 1, color);
}

void memoryLcdDemo(void)
{
	initMemoryLCD();

	clearDisplay();
	lcdWidth = getDisplayWidth();
	lcdHeight = getDisplayHeight();
	sleep(2);

	UG_GUI gui;
	UG_Init(&gui, warpWritePixelToFrameBuffer, 144, 168);
	
	while (1)
	{
		// fill screen with black
		UG_FillScreen(C_BLACK);
		writeFrameBufferToDisplay();
		sleep(500);
		clearFrameBuffer();
		clearDisplay();

		// fill screen with white
		UG_FillScreen(C_WHITE);
		writeFrameBufferToDisplay();
		sleep(500);
		clearFrameBuffer();
		clearDisplay();

		// draw some shape
		UG_DrawFrame(16, 16, 32, 32, C_BLACK);
		UG_FillFrame(48, 16, 64, 32, C_BLACK);
		UG_DrawCircle(88, 24, 8, C_BLACK);
		UG_FillCircle(120, 24, 8, C_BLACK);

		UG_DrawRoundFrame(16, 16 * 3, 16 + 16 * 3, 16 * 6, 10, C_BLACK);
		UG_DrawMesh(16 * 5, 16 * 3, 16 * 8, 16 * 6, C_BLACK);

		for (uint8_t i = 0; i < LCDWIDTH - 16; i++)
		{
			if (i % 4 == 0)
			{
				UG_DrawPixel(16 + i, 16 * 7, C_BLACK);
			}
		}

		UG_DrawLine(16, 16 * 8, LCDWIDTH - 16, 16 * 8, C_BLACK);

		writeFrameBufferToDisplay();
		sleep(2000);
		clearFrameBuffer();
		clearDisplay();

		// draw string
		UG_SetBackcolor(C_WHITE);
		UG_SetForecolor(C_BLACK);
		UG_FontSelect(&FONT_8X12);
		UG_PutString(20, 50, "Hello World!");
		UG_FontSelect(&FONT_10X16);
		UG_PutString(20, 100, "WAVESHARE");
		writeFrameBufferToDisplay();
		sleep(2000);
		clearFrameBuffer();
		clearDisplay();

		// draw an image
		drawImage((unsigned char *)gImage_pic);
		writeFrameBufferToDisplay();
		sleep(2000);
		clearFrameBuffer();
		clearDisplay();
	}
}
