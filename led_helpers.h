// this file hold functions etc used to manage the LED

#include <Adafruit_NeoPixel.h>

// this defines the data pin used to drive the LED(s)
#define LEDPIN    D3
#define MAINLED   0
#define LEDCYCLE  200

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, LEDPIN, NEO_GRB + NEO_KHZ800);

uint32_t LED_Colour1 = LED_blank;
uint32_t LED_Colour2 = LED_blank;
uint32_t LED_Store = LED_blank;

long int led_timer = 0;
int led_cycle = 1;
bool led_single = false;

/*
   Function to be called from main script to control LED(s)
*/
void showLED()
{
  if (led_cycle == 1)     // first cycle
  {
    //pixels.setPixelColor(MAINLED, LED_Colour1);
    pixels.setPixelColor(MAINLED, LED_Colour1);
  }
  else                    // must be second cycle
  {
    pixels.setPixelColor(MAINLED, LED_Colour2);
  }
  pixels.show();
  if ((led_timer + LEDCYCLE) < millis())
  {
    led_timer = millis();
    if (led_cycle == 1)
    {
      led_cycle = 2;
    }
    else
    {
      led_cycle = 1;
    }
    if (led_single == true)
    {
      led_single = false;
      setPrimaryLED(LED_Store);
      //setSecondaryLED(LED_blank);
    }
  }
}

void setPrimaryLED(uint32_t pColour)
{
  LED_Colour1 = pColour;
}

void setSecondaryLED(uint32_t pColour)
{
  LED_Colour2 = pColour;
}

/*
 * Sets up for a single blink
 */
void singleLEDblink(uint32_t pColour)
{
  LED_Store = LED_Colour1;
  setPrimaryLED(pColour);
  led_single = true;
  led_timer = millis();
  //Serial.println(__FUNCTION__);
}
