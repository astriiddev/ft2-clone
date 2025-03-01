#pragma once

#include <stdint.h>
#include <stdbool.h>

enum
{
	MOUSE_MODE_NORMAL = 0,
	MOUSE_MODE_DELETE = 1,
	MOUSE_MODE_RENAME = 2,

	MOUSE_WHEEL_DOWN = 0,
	MOUSE_WHEEL_UP = 1,

	MOUSE_WHEEL_LEFT = 0,
	MOUSE_WHEEL_RIGHT = 1
};

typedef struct mouse_t
{
	volatile bool setPosFlag;
	bool leftButtonPressed, rightButtonPressed, leftButtonReleased, rightButtonReleased;
	bool firstTimePressingButton, mouseOverTextBox;
	int8_t buttonCounter, mode;
	int16_t lastUsedObjectID, lastUsedObjectType, lastEditBox;
	int32_t absX, absY, rawX, rawY, x, y, lastX, lastY, xBias, yBias, setPosX, setPosY;
	int32_t lastScrollX, lastScrollXTmp, lastScrollY, saveMouseX, saveMouseY;
	uint32_t buttonState;
} mouse_t;

extern mouse_t mouse; // ft2_mouse.c

// do not change these!
#define MOUSE_CURSOR_W 26
#define MOUSE_CURSOR_H 23
#define MOUSE_GLASS_ANI_FRAMES 22
#define MOUSE_CLOCK_ANI_FRAMES 5

void freeMouseCursors(void);
bool createMouseCursors(void);
void setMousePosToCenter(void);
void setMouseShape(int16_t shape);
void setMouseMode(uint8_t mode);
void mouseWheelHandler(bool directionUp);
void mouseWheelSampScroll(const bool directionRight);
void mouseButtonUpHandler(uint8_t mouseButton);
void mouseButtonDownHandler(uint8_t mouseButton);
void updateMouseScaling(void);
void setMouseBusy(bool busy); // can be called from other threads
void mouseAnimOn(void);
void mouseAnimOff(void);
void animateBusyMouse(void);
void handleLastGUIObjectDown(void);
void readMouseXY(void);
void resetMouseBusyAnimation(void);
