/* This example implements Conway's Game of Life on an
 * APA102-based LED panel.
 *
 * For more information about the Game of Life, see:
 *
 *     https://en.wikipedia.org/wiki/Game_of_Life
 *
 * The top of this file defines several settings which you can
 * edit.  Be sure to edit the width and height variables to
 * match your LED panel.  For the 8x32 panel from Pololu,
 * ledPanelWidth should be 8 and ledPanelHeight should be 32.
 *
 * By default, this example initializes each cell in the game to
 * a random state.  There are some other options for the initial
 * state of the game that you can use by editing the setup()
 * function. */

/* This example is meant for controlling large numbers of LEDs,
 * so it requires the FastGPIO library.  If you cannot use the
 * FastGPIO library, you can comment out the two lines below and
 * the example will still work, but it will be slow. */
#include <FastGPIO.h>
#define APA102_USE_FAST_GPIO

#include <APA102.h>

// Define which pins to use.
const uint8_t dataPin = 11;
const uint8_t clockPin = 12;
const uint8_t analogRandomSeedPin = A0;

// Set the size of the LED panel (and the game).
const uint8_t width = 16;
const uint8_t height = 16;

// Set whether the left and right edges are considered to
// border eachother.
const bool wraparoundX = true;

// Set whether the top and bottom edges are considered to border
// eachother.
const bool wraparoundY = true;

// Determines whether we display the age of the live cells using
// different colors.  If false, all live cells are the same
// color.
const bool showAge = true;

// Set the brightness to use (the maximum is 31).
const uint8_t brightness = 1;

// Set how long to display each frame (in milliseconds).
const uint16_t frameTime = 100;

// Create an object for writing to the LED strip.
APA102<dataPin, clockPin> ledStrip;

const uint16_t cellCount = width * height;

// Make a buffer for the game state.  Each cell gets one byte.
// Bit 7: 1 if the cell is alive currently.
// Bit 6: Temporarily 1 if the cell will be alive
//        in the next frame.
// Bits 0-5: the age of the cell, if alive currently,
//    capped at 64.  Otherwise, these will be 0.
uint8_t gameState[cellCount] = {0};

#define STATE_ALIVE       0x80
#define STATE_ALIVE_NEXT  0x40
#define STATE_DEAD        0x00
#define STATE_AGE_MASK    0x3F

// Returns a reference to the byte representing the state of a
// specific cell.  If x or y are invalid, returns a reference to
// a dummy byte that is set to 0 (not alive).
uint8_t & cellState(uint8_t x, uint8_t y)
{
  if (wraparoundX)
  {
    if (x == 0xFF) { x = width - 1; }
    if (x == width) { x = 0; }
  }

  if (wraparoundY)
  {
    if (y == 0xFF) { y = height - 1; }
    if (y == height) { y = 0; }
  }

  if (x >= width || y >= height)
  {
    static uint8_t dummy;
    dummy = 0;
    return dummy;
  }

  return gameState[y * width + x];
}

void setupRandom()
{
  randomSeed(analogRead(analogRandomSeedPin));
  for (uint16_t i = 0; i < cellCount; i++)
  {
    gameState[i] = random(2) ? STATE_ALIVE : STATE_DEAD;
  }
}

// Sets up a glider that will travel in the positive x, positive
// y direction, with its nose starting at the specified
// coordinates.
void setupGlider(uint8_t x, uint8_t y)
{
  cellState(x, y) = STATE_ALIVE;
  cellState(x - 1, y) = STATE_ALIVE;
  cellState(x - 2, y) = STATE_ALIVE;
  cellState(x, y - 1) = STATE_ALIVE;
  cellState(x - 1, y - 2) = STATE_ALIVE;
}

// Sets up a light-weight space ship that will travel in the
// positive x direction, with its nose starting at the specified
// coordinates.
void setupLightweightSpaceship(uint8_t x, uint8_t y)
{
  cellState(x, y) = STATE_ALIVE;
  cellState(x - 1, y) = STATE_ALIVE;
  cellState(x - 2, y) = STATE_ALIVE;
  cellState(x - 3, y) = STATE_ALIVE;
  cellState(x - 4, y + 1) = STATE_ALIVE;
  cellState(x - 4, y + 3) = STATE_ALIVE;
  cellState(x - 1, y + 3) = STATE_ALIVE;
  cellState(x, y + 2) = STATE_ALIVE;
  cellState(x, y + 1) = STATE_ALIVE;
}

/* Include an XBM image.  XBM is an image format that is compatible
 * with C/C++ code and can store monochrome bitmaps.  This is
 * used by setupFromXBM().  The default image is an R pentomino, but
 * you can use an image editor such as GIMP to save a new image. */
#include "start.cpp"

/* Initializes the game state using an XBM image.  Uses
 * start_height, start_width, and start_bits, which are defined
 * in start.xbm, which is included above.  Places the image in
 * the upper left corner of the LED panel.  It is OK if the image
 * is no the same size as the LED panel. */
void setupFromXBM()
{
  const uint8_t bytesPerRow = (start_width + 7) / 8;
  for (uint8_t y = 0; y < start_height; y++)
  {
    for (uint8_t x = 0; x < start_width; x++)
    {
      if (start_bits[bytesPerRow * y + (x / 8)] >> (x % 8) & 1)
      {
        cellState(x, y) = STATE_ALIVE;
      }
      else
      {
        cellState(x, y) = STATE_DEAD;
      }
    }
  }
}

void setup()
{
  // By default, we use a random game state.  You can comment out
  // the call to setupRandom() and uncomment one of the other
  // calls if you want to see a different pattern used as the
  // initial state.

  //setupGlider(4, 3);
  //setupLightweightSpaceship(4, 3);
  //setupFromXBM();
  setupRandom();
}

// Writes a specific cell to the LED panel.
void writeCellToLed(uint8_t cellState)
{
  if (cellState & STATE_ALIVE)
  {
    // The cell is alive.  Send a color that depends on its age.
    uint8_t age = cellState & STATE_AGE_MASK;

    if (age == 0 || !showAge)
    {
      ledStrip.sendColor(0x00, 0x00, 0xFF, brightness);
    }
    else if (age == 1)
    {
      ledStrip.sendColor(0x40, 0x00, 0xFF, brightness);
    }
    else if (age == 2)
    {
      ledStrip.sendColor(0xFF, 0x00, 0xFF, brightness);
    }
    else if (age == 3)
    {
      ledStrip.sendColor(0xFF, 0x00, 0x40, brightness);
    }
    else
    {
      // Fade from red to green as age goes from 4 to 63.
      uint8_t phase = (age - 4) * 255 / (63 - 4);
      ledStrip.sendColor(0xFF - phase, phase, 0x00, brightness);
    }
  }
  else
  {
    // The cell is dead.
    ledStrip.sendColor(0, 0, 0, brightness);
  }
}

// Writes the overall state of the game to the LED panel.
void writeGameStateToLeds()
{
  ledStrip.startFrame();
  for (uint8_t y = 0; y < height; y++)
  {
    if (y & 1)
    {
      // For odd rows, write the row without reversing it.
      for (uint8_t x = 0; x < width; x++)
      {
        writeCellToLed(cellState(x, y));
      }
    }
    else
    {
      // For even rows, write the row in reverse order because
      // the LED panel uses a serpentine layout.
      for (uint8_t x = 0; x < width; x++)
      {
        writeCellToLed(cellState(width - 1 - x, y));
      }
    }
  }
  ledStrip.endFrame(cellCount);
}

void updateGameState()
{
  // In this first pass, we update bit 6 of each cell, setting it
  // to 1 if the cell will be alive in the next frame and 0 if
  // the cell will be dead.  We also update the age for surviving
  // live cells.
  for (uint8_t y = 0; y < height; y++)
  {
    for (uint8_t x = 0; x < width; x++)
    {
      uint8_t neighborCount =
        (cellState(x - 1, y - 1) >> 7) +
        (cellState(x - 1, y + 0) >> 7) +
        (cellState(x - 1, y + 1) >> 7) +
        (cellState(x + 0, y - 1) >> 7) +
        (cellState(x + 0, y + 1) >> 7) +
        (cellState(x + 1, y - 1) >> 7) +
        (cellState(x + 1, y + 0) >> 7) +
        (cellState(x + 1, y + 1) >> 7);

      if (cellState(x, y) & STATE_ALIVE)
      {
        // The cell is alive.
        if (neighborCount < 2 || neighborCount > 3)
        {
          // The cell dies on the next frame
          // (clear the STATE_ALIVE_NEXT bit and the age).
          cellState(x, y) = STATE_ALIVE;
        }
        else
        {
          // The cell survives to the next generation.
          // Increment its age and record survival.
          uint8_t age = cellState(x, y) & STATE_AGE_MASK;
          if (age < STATE_AGE_MASK) { age += 1; }
          cellState(x, y) = STATE_ALIVE | STATE_ALIVE_NEXT | age;
        }
      }
      else
      {
        // The cell is dead.
        if (neighborCount == 3)
        {
          // The cell will be born on the next frame.
          cellState(x, y) = STATE_ALIVE_NEXT;
        }
        else
        {
          // The cell remains dead.
          cellState(x, y) = STATE_DEAD;
        }
      }
    }
  }

  // In this second pass, we update the actual liveness state
  // (in bit 7) by copying bit 6 to bit 7.
  for (uint16_t i = 0; i < cellCount; i++)
  {
    if (gameState[i] & STATE_ALIVE_NEXT)
    {
      gameState[i] |= STATE_ALIVE;
    }
    else
    {
      gameState[i] &= ~STATE_ALIVE;
    }
  }
}

void loop()
{
  writeGameStateToLeds();
  updateGameState();
  delay(frameTime);
}
