/*
  This sketch runs on Lilygo T-Display S3 device (and also T-Embed by changing Pins).

  It is  a  complete  radio  capable  to  tune  LW,  MW,  SW  on  AM  and  SSB  mode  and  also  receive  the
  regular  comercial  stations.

  Features:   AM; SSB; LW/MW/SW; external mute circuit control; AGC; Attenuation gain control;
              SSB filter; CW; AM filter; 1, 5, 10, 50 and 500kHz step on AM and 10Hhz sep on SSB

  Lilygo T-Display S3
  
    |--------------|------------|------------|------------|
    |   Lilygo     |   Si4735   |  Encoder   |   Audio    |
    | T-Display S3 |            |            | Amplifier  |        
    |--------------|------------|------------|------------|        
    |     3V3      |    Vcc     |            |    Vcc     |        
    |     GND      |    GND     |     2,4    |    GND     |        Encoder        1,2,3        
    |     21       |            |     5      |            |        Encoder switch 4,5
    |     16       |   Reset    |            |            |
    |     43       |    SDA     |            |            |
    |     44       |    SCL     |            |            |
    |      1       |            |      1     |            |
    |      2       |            |      3     |            |
    |              |    LOut    |            |    LIn     |
    |              |    ROut    |            |    RIn     |
    |     17 Mute  |            |            |    Mute    |
    |--------------|------------|------------|------------|
  
  
    Lilygo T-Embed
  
    |--------------|------------|------------|
    |    Lilygo    |   Si4735   |   Audio    |
    |   T-Embeded  |            | Amplifier  |
    |--------------|------------|------------|
    |     3V3      |    Vcc     |    Vcc     |
    |     GND      |    GND     |    GND     |
    |              |            |            |
    |     16       |   Reset    |            |
    |     18       |    SDA     |            |
    |      8       |    SCL     |            |
    |              |            |            |
    |              |            |            |
    |              |    LOut    |    LIn     |
    |              |    ROut    |    RIn     |
    |     17 Mute  |            |    Mute    |
    |--------------|------------|------------|
  
  (*1) If you are using the SI4732-A10, check the corresponding pin numbers.
  (*2) If you are using the Lilygo T-Embeded, check the corresponding pin numbers.
  (*3) The PU2CLR SI4735 Arduino Library has resources to detect the I2C bus address automatically.
       It seems the original project connect the SEN pin to the +Vcc. By using this sketch, you do
       not need to worry about this setting.
  Prototype documentation: https://pu2clr.github.io/SI4735/
  PU2CLR Si47XX API documentation: https://pu2clr.github.io/SI4735/extras/apidoc/html/

  By PU2CLR, Ricardo, May  2021.
  Modded by Ralph Xavier, Jan 2023
*/

// SI4735_Shield example form https://github.com/ralphxavier/SI4735
#include <Wire.h>
#include <TFT_eSPI.h>
#include "EEPROM.h"
#include <SI4735.h>
#include <Battery18650Stats.h> // https://github.com/danilopinotti/Battery18650Stats
#include <OneButton.h>  // https://github.com/mathertel/OneButton

#include "Rotary.h"
#include "patch_init.h" // SSB patch for whole SSBRX initialization string

const uint16_t size_content = sizeof ssb_patch_content; // see patch_init.h

#define FM_BAND_TYPE 0
#define MW_BAND_TYPE 1
#define SW_BAND_TYPE 2
#define LW_BAND_TYPE 3

#define PIN_POWER_ON  46
#define PIN_LCD_BL    15
#define RESET_PIN     16           
#define AUDIO_MUTE    17           

// Enconder PINs
#define ENCODER_PIN_A  2           // GPIO01 
#define ENCODER_PIN_B  1           // GPIO02

// I2C bus pin on Lilygo T-Display
#define ESP32_I2C_SDA 18           // GPIO43 
#define ESP32_I2C_SCL 8           // GPIO44

//Battery Monitor
#define VBAT_MON         4                // GPIO04
#define MIN_USB_VOLTAGE  4.9
#define CONV_FACTOR      1.8
#define READS            20

// Buttons controllers
#define ENCODER_PUSH_BUTTON     0     // GPIO21

#define MIN_ELAPSED_TIME         5  //300
#define MIN_ELAPSED_RSSI_TIME  200
#define ELAPSED_COMMAND       2000  // time to turn off the last command controlled by encoder. Time to goes back to the FVO control
#define ELAPSED_CLICK         1500  // time to check the double click commands
#define DEFAULT_VOLUME          35  // change it for your favorite sound volume
#define STRENGTH_CHECK_TIME   1500
#define RDS_CHECK_TIME          90

#define FM  0
#define LSB 1
#define USB 2
#define AM  3
#define LW  4

#define SSB 1

#define VOLUME       0
#define STEP         1
#define MODE         2
#define BFO          3 
#define BW           4
#define AGC_ATT      5
#define SOFTMUTE     6
#define SEEKUP       7
#define SEEKDOWN     8
#define BAND         9
#define MUTE        10

#define TFT_MENU_BACK TFT_BLACK  // 0x01E9
#define TFT_MENU_HIGHLIGHT_BACK TFT_BLUE

#define EEPROM_SIZE        512

#define STORE_TIME 10000 // Time of inactivity to make the current receiver status writable (10s / 10000 milliseconds).

// EEPROM - Stroring control variables
const uint8_t app_id = 47; // Useful to check the EEPROM content before processing useful data
const int eeprom_address = 0;
long storeTime = millis();

bool itIsTimeToSave = false;

bool bfoOn = false;
bool ssbLoaded = false;
char bfo[18]="0000";
bool muted = false;
int8_t agcIdx = 0;
uint8_t disableAgc = 0;
int8_t agcNdx = 0;
int8_t softMuteMaxAttIdx = 4;
uint8_t countClick = 0;

uint8_t seekDirection = 1;

bool cmdBand = false;
bool cmdVolume = false;
bool cmdAgc = false;
bool cmdBandwidth = false;
bool cmdStep = false;
bool cmdMode = false;
bool cmdMenu = false;
bool cmdSoftMuteMaxAtt = false;

bool fmRDS = false;

int16_t currentBFO = 0;
long elapsedRSSI = millis();
long elapsedButton = millis();

long lastStrengthCheck = millis();
long lastRDSCheck = millis();

long elapsedClick = millis();
long elapsedCommand = millis();
volatile int encoderCount = 0;
uint16_t currentFrequency;

const uint8_t currentBFOStep = 10;

char sAgc[15];

const char *menu[] = {"Volume", "Step", "Mode", "BFO", "BW", "AGC/Att", "SoftMute", "Seek Up", "Seek Dn", "Band", "Mute"};
int8_t menuIdx = VOLUME;
const int lastMenu = (sizeof menu / sizeof(char *)) - 1;
int8_t currentMenuCmd = -1;

typedef struct
{
  uint8_t idx;      // SI473X device bandwidth index
  const char *desc; // bandwidth description
} Bandwidth;

int8_t bwIdxSSB = 4;
const int8_t maxSsbBw = 5;
Bandwidth bandwidthSSB[] = {
  {4, "0.5"},
  {5, "1.0"},
  {0, "1.2"},
  {1, "2.2"},
  {2, "3.0"},
  {3, "4.0"}
};
const int lastBandwidthSSB = (sizeof bandwidthSSB / sizeof(Bandwidth)) - 1;

int8_t bwIdxAM = 4;
const int8_t maxAmBw = 6;
Bandwidth bandwidthAM[] = {
  {4, "1.0"},
  {5, "1.8"},
  {3, "2.0"},
  {6, "2.5"},
  {2, "3.0"},
  {1, "4.0"},
  {0, "6.0"}
};
const int lastBandwidthAM = (sizeof bandwidthAM / sizeof(Bandwidth)) - 1;

int8_t bwIdxFM = 0;
const int8_t maxFmBw = 4;

Bandwidth bandwidthFM[] = {
    {0, "AUT"}, // Automatic - default
    {1, "110"}, // Force wide (110 kHz) channel filter.
    {2, " 84"},
    {3, " 60"},
    {4, " 40"}};
const int lastBandwidthFM = (sizeof bandwidthFM / sizeof(Bandwidth)) - 1;



int tabAmStep[] = {1,    // 0
                   5,    // 1
                   9,    // 2
                   10,   // 3
                   50,   // 4
                   100}; // 5

const int lastAmStep = (sizeof tabAmStep / sizeof(int)) - 1;
int idxAmStep = 3;

int tabFmStep[] = {5, 10, 20};
const int lastFmStep = (sizeof tabFmStep / sizeof(int)) - 1;
int idxFmStep = 1;

uint16_t currentStepIdx = 1;


const char *bandModeDesc[] = {"FM ", "LSB", "USB", "AM "};
const int lastBandModeDesc = (sizeof bandModeDesc / sizeof(char *)) - 1;
uint8_t currentMode = FM;


/**
 *  Band data structure
 */
typedef struct
{
  const char *bandName;   // Band description
  uint8_t bandType;       // Band type (FM, MW or SW)
  uint16_t minimumFreq;   // Minimum frequency of the band
  uint16_t maximumFreq;   // maximum frequency of the band
  uint16_t currentFreq;   // Default frequency or current frequency
  int8_t currentStepIdx;  // Idex of tabStepAM:  Defeult frequency step (See tabStepAM)
  int8_t bandwidthIdx;    // Index of the table bandwidthFM, bandwidthAM or bandwidthSSB;
} Band;

/*
   Band table
   YOU CAN CONFIGURE YOUR OWN BAND PLAN. Be guided by the comments.
   To add a new band, all you have to do is insert a new line in the table below. No extra code will be needed.
   You can remove a band by deleting a line if you do not want a given band. 
   Also, you can change the parameters of the band.
   ATTENTION: You have to RESET the eeprom after adding or removing a line of this table. 
              Turn your receiver on with the encoder push button pressed at first time to RESET the eeprom content.  
*/
Band band[] = {
    {"VHF", FM_BAND_TYPE, 6400, 10800, 10390, 1, 0},
    {"MW1", MW_BAND_TYPE, 150, 1720, 810, 3, 4},
    {"MW2", MW_BAND_TYPE, 531, 1701, 783, 2, 4},
    {"MW2", MW_BAND_TYPE, 1700, 3500, 2500, 1, 4},
    {"80M", MW_BAND_TYPE, 3500, 4000, 3700, 0, 4},
    {"SW1", SW_BAND_TYPE, 4000, 5500, 4885, 1, 4},
    {"SW2", SW_BAND_TYPE, 5500, 6500, 6000, 1, 4},
    {"40M", SW_BAND_TYPE, 6500, 7300, 7100, 0, 4},
    {"SW3", SW_BAND_TYPE, 7200, 8000, 7200, 1, 4},
    {"SW4", SW_BAND_TYPE, 9000, 11000, 9500, 1, 4},
    {"SW5", SW_BAND_TYPE, 11100, 13000, 11900, 1, 4},
    {"SW6", SW_BAND_TYPE, 13000, 14000, 13500, 1, 4},
    {"20M", SW_BAND_TYPE, 14000, 15000, 14200, 0, 4},
    {"SW7", SW_BAND_TYPE, 15000, 17000, 15300, 1, 4},
    {"SW8", SW_BAND_TYPE, 17000, 18000, 17500, 1, 4},
    {"15M", SW_BAND_TYPE, 20000, 21400, 21100, 0, 4},
    {"SW9", SW_BAND_TYPE, 21400, 22800, 21500, 1, 4},
    {"CB ", SW_BAND_TYPE, 26000, 28000, 27500, 0, 4},
    {"10M", SW_BAND_TYPE, 28000, 30000, 28400, 0, 4},
    {"ALL", SW_BAND_TYPE, 150, 30000, 15000, 0, 4} // All band. LW, MW and SW (from 150kHz to 30MHz)
};                                             

const int lastBand = (sizeof band / sizeof(Band)) - 1;
int bandIdx = 0;
int tabStep[] = {1, 5, 10, 50, 100, 500, 1000};
const int lastStep = (sizeof tabStep / sizeof(int)) - 1;

char *rdsMsg;
char *stationName;
char *rdsTime;
char bufferStationName[50];
char bufferRdsMsg[100];
char bufferRdsTime[32];

uint8_t rssi = 0;
uint8_t snr = 0;
uint8_t volume = DEFAULT_VOLUME;

int XbatPos = 290;  // Position of battery icon
int YbatPos =   6;
int Xbatsiz =  20;  // size of battery icon
int Ybatsiz =   9;
int previousBatteryLevel = -1;
int currentBatteryLevel = 1;
bool batteryCharging = false;

// Devices class declarations
Rotary encoder = Rotary(ENCODER_PIN_A, ENCODER_PIN_B);

Battery18650Stats battery(VBAT_MON);

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

SI4735 rx;

void setup()
{

  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);

  // Encoder pins
  pinMode(ENCODER_PUSH_BUTTON, INPUT_PULLUP);
  
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);

  // The line below may be necessary to setup I2C pins on ESP32
  Wire.begin(ESP32_I2C_SDA, ESP32_I2C_SCL);


  tft.begin();

  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  spr.createSprite(320,170);
  spr.setTextDatum(MC_DATUM);
  spr.setSwapBytes(true);
  spr.setFreeFont(&Orbitron_Light_24);
  spr.setTextColor(TFT_WHITE,TFT_BLACK);

  ledcSetup(0, 2000, 8);
  ledcAttachPin(PIN_LCD_BL, 0);
  ledcWrite(0, 255);

/*  // Splash - Remove or change it for your introduction text.
  display.clearDisplay();
  print(0, 0, NULL, 2, "PU2CLR");
  print(0, 15, NULL, 2, "ESP32");
  display.display();
  delay(2000);
  display.clearDisplay();
  print(0, 0, NULL, 2, "SI473X");
  print(0, 15, NULL, 2, "Arduino");
  display.display();
  // End Splash

  delay(2000);
  display.clearDisplay();
*/

  EEPROM.begin(EEPROM_SIZE);

  // If you want to reset the eeprom, keep the VOLUME_UP button pressed during statup
  if (digitalRead(ENCODER_PUSH_BUTTON) == LOW)
  {
    EEPROM.write(eeprom_address, 0);
    EEPROM.commit();
    tft.setTextSize(2);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("EEPROM RESETED");
    delay(3000);
    tft.fillScreen(TFT_BLACK);
  }

  // ICACHE_RAM_ATTR void rotaryEncoder(); see rotaryEncoder implementation below.
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), rotaryEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), rotaryEncoder, CHANGE);

  rx.setI2CFastModeCustom(100000);
  
  int16_t si4735Addr = rx.getDeviceI2CAddress(RESET_PIN); // Looks for the I2C bus address and set it.  Returns 0 if error

  if ( si4735Addr == 0 ) {
    tft.setTextSize(2);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("Si4735 not detected");
    while (1);
  }  
  
  rx.setup(RESET_PIN, MW_BAND_TYPE);
  // Comment the line above and uncomment the three lines below if you are using external ref clock (active crystal or signal generator)
  // rx.setRefClock(32768);
  // rx.setRefClockPrescaler(1);   // will work with 32768  
  // rx.setup(RESET_PIN, 0, MW_BAND_TYPE, SI473X_ANALOG_AUDIO, XOSCEN_RCLK);

  rx.setAudioMuteMcuPin(AUDIO_MUTE);  
  
  cleanBfoRdsInfo();
  
  delay(300);


  // Checking the EEPROM content
  if (EEPROM.read(eeprom_address) == app_id)
  {
    readAllReceiverInformation();
  } else 
    rx.setVolume(volume);

  useBand();
  
  showStatus();
  drawSprite();
}


/**
 * Prints a given content on display 
 */
void print(uint8_t col, uint8_t lin, const GFXfont *font, uint8_t textSize, const char *msg) {
  tft.setCursor(col,lin);
  tft.setTextSize(textSize);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.println(msg);
}

void printParam(const char *msg) {
 tft.fillScreen(TFT_BLACK);
 print(0,10,NULL,2, msg);
 }

/*
   writes the conrrent receiver information into the eeprom.
   The EEPROM.update avoid write the same data in the same memory position. It will save unnecessary recording.
*/
void saveAllReceiverInformation()
{
  int addr_offset;

  EEPROM.begin(EEPROM_SIZE);

  EEPROM.write(eeprom_address, app_id);                 // stores the app id;
  EEPROM.write(eeprom_address + 1, rx.getVolume()); // stores the current Volume
  EEPROM.write(eeprom_address + 2, bandIdx);            // Stores the current band
  EEPROM.write(eeprom_address + 3, fmRDS);
  EEPROM.write(eeprom_address + 4, currentMode); // Stores the current Mode (FM / AM / SSB)
  EEPROM.write(eeprom_address + 5, currentBFO >> 8);
  EEPROM.write(eeprom_address + 6, currentBFO & 0XFF);
  EEPROM.commit();

  addr_offset = 7;
  band[bandIdx].currentFreq = currentFrequency;

  for (int i = 0; i <= lastBand; i++)
  {
    EEPROM.write(addr_offset++, (band[i].currentFreq >> 8));   // stores the current Frequency HIGH byte for the band
    EEPROM.write(addr_offset++, (band[i].currentFreq & 0xFF)); // stores the current Frequency LOW byte for the band
    EEPROM.write(addr_offset++, band[i].currentStepIdx);       // Stores current step of the band
    EEPROM.write(addr_offset++, band[i].bandwidthIdx);         // table index (direct position) of bandwidth
    EEPROM.commit();
  }

  EEPROM.end();
}

/**
 * reads the last receiver status from eeprom. 
 */
void readAllReceiverInformation()
{
  uint8_t volume;
  int addr_offset;
  int bwIdx;
  EEPROM.begin(EEPROM_SIZE);

  volume = EEPROM.read(eeprom_address + 1); // Gets the stored volume;
  bandIdx = EEPROM.read(eeprom_address + 2);
  fmRDS = EEPROM.read(eeprom_address + 3);
  currentMode = EEPROM.read(eeprom_address + 4);
  currentBFO = EEPROM.read(eeprom_address + 5) << 8;
  currentBFO |= EEPROM.read(eeprom_address + 6);

  addr_offset = 7;
  for (int i = 0; i <= lastBand; i++)
  {
    band[i].currentFreq = EEPROM.read(addr_offset++) << 8;
    band[i].currentFreq |= EEPROM.read(addr_offset++);
    band[i].currentStepIdx = EEPROM.read(addr_offset++);
    band[i].bandwidthIdx = EEPROM.read(addr_offset++);
  }

  EEPROM.end();

  currentFrequency = band[bandIdx].currentFreq;

  if (band[bandIdx].bandType == FM_BAND_TYPE)
  {
    currentStepIdx = idxFmStep = band[bandIdx].currentStepIdx;
    rx.setFrequencyStep(tabFmStep[currentStepIdx]);
  }
  else
  {
    currentStepIdx = idxAmStep = band[bandIdx].currentStepIdx;
    rx.setFrequencyStep(tabAmStep[currentStepIdx]);
  }

  bwIdx = band[bandIdx].bandwidthIdx;

  if (currentMode == LSB || currentMode == USB)
  {
    loadSSB();
    bwIdxSSB = (bwIdx > 5) ? 5 : bwIdx;
    rx.setSSBAudioBandwidth(bandwidthSSB[bwIdxSSB].idx);
    // If audio bandwidth selected is about 2 kHz or below, it is recommended to set Sideband Cutoff Filter to 0.
    if (bandwidthSSB[bwIdxSSB].idx == 0 || bandwidthSSB[bwIdxSSB].idx == 4 || bandwidthSSB[bwIdxSSB].idx == 5)
      rx.setSSBSidebandCutoffFilter(0);
    else
      rx.setSSBSidebandCutoffFilter(1);
    rx.setSSBBfo(currentBFO);      
  }
  else if (currentMode == AM)
  {
    bwIdxAM = bwIdx;
    rx.setBandwidth(bandwidthAM[bwIdxAM].idx, 1);
  }
  else
  {
    bwIdxFM = bwIdx;
    rx.setFmBandwidth(bandwidthFM[bwIdxFM].idx);
  }

  if (currentBFO > 0)
    sprintf(bfo, "+%4.4d", currentBFO);
  else
    sprintf(bfo, "%4.4d", currentBFO);

  delay(50);
  rx.setVolume(volume);
}

/*
 * To store any change into the EEPROM, it is needed at least STORE_TIME  milliseconds of inactivity.
 */
void resetEepromDelay()
{
  elapsedCommand = storeTime = millis();
  itIsTimeToSave = true;
}

/**
    Set all command flags to false
    When all flags are disabled (false), the encoder controls the frequency
*/
void disableCommands()
{
  cmdBand = false;
  bfoOn = false;
  cmdVolume = false;
  cmdAgc = false;
  cmdBandwidth = false;
  cmdStep = false;
  cmdMode = false;
  cmdMenu = false;
  cmdSoftMuteMaxAtt = false;
  countClick = 0;
  // showCommandStatus((char *) "VFO ");
}

/**
 * Reads encoder via interrupt
 * Use Rotary.h and  Rotary.cpp implementation to process encoder via interrupt
 * if you do not add ICACHE_RAM_ATTR declaration, the system will reboot during attachInterrupt call. 
 * With ICACHE_RAM_ATTR macro you put the function on the RAM.
 */
ICACHE_RAM_ATTR void  rotaryEncoder()
{ // rotary encoder events
  uint8_t encoderStatus = encoder.process();
  if (encoderStatus)
    encoderCount = (encoderStatus == DIR_CW) ? 1 : -1;
}

/**
 * Shows frequency information on Display
 */
void showFrequency()
{
  char tmp[15];
  sprintf(tmp, "%5.5u", currentFrequency);
  drawSprite();
  // showMode();
}

/**
 * Shows the current mode
 */
void showMode() {
  drawSprite();    
}

/**
 * Shows some basic information on display
 */
void showStatus()
{
  showFrequency();
  showRSSI();
}

/**
 *  Shows the current Bandwidth status
 */
void showBandwidth()
{
  drawSprite();
}

/**
 *   Shows the current RSSI and SNR status
 */
void showRSSI()
{
  char sMeter[10];
  sprintf(sMeter, "S:%d ", rssi);
  drawSprite();
}

/**
 *    Shows the current AGC and Attenuation status
 */
void showAgcAtt()
{
  // lcd.clear();
  rx.getAutomaticGainControl();
  if (agcNdx == 0 && agcIdx == 0)
    strcpy(sAgc, "AGC ON");
  else
    sprintf(sAgc, "ATT: %2.2d", agcNdx);

  drawSprite();

}

/**
 *   Shows the current step
 */
void showStep()
{
  drawSprite();
}

/**
 *  Shows the current BFO value
 */
void showBFO()
{
  
  if (currentBFO > 0)
    sprintf(bfo, "+%4.4d", currentBFO);
  else
    sprintf(bfo, "%4.4d", currentBFO);
  drawSprite();
  elapsedCommand = millis();
}

/*
 *  Shows the volume level on LCD
 */
void showVolume()
{
drawSprite();
}

/**
 * Show Soft Mute 
 */
void showSoftMute()
{
  drawSprite();
}

/**
 *   Sets Band up (1) or down (!1)
 */
void setBand(int8_t up_down)
{
  band[bandIdx].currentFreq = currentFrequency;
  band[bandIdx].currentStepIdx = currentStepIdx;
  if (up_down == -1)
    bandIdx = (bandIdx < lastBand) ? (bandIdx + 1) : 0;
  else
    bandIdx = (bandIdx > 0) ? (bandIdx - 1) : lastBand;
  useBand();
  delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
  elapsedCommand = millis();
}

/**
 * Switch the radio to current band
 */
void useBand()
{
  if (band[bandIdx].bandType == FM_BAND_TYPE)
  {
    currentMode = FM;
    rx.setTuneFrequencyAntennaCapacitor(0);
    rx.setFM(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq, band[bandIdx].currentFreq, tabFmStep[band[bandIdx].currentStepIdx]);
    rx.setSeekFmLimits(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq);
    bfoOn = ssbLoaded = false;
    bwIdxFM = band[bandIdx].bandwidthIdx;
    rx.setFmBandwidth(bandwidthFM[bwIdxFM].idx);
    rx.setFMDeEmphasis(1);
    rx.RdsInit();
    rx.setRdsConfig(1, 2, 2, 2, 2);
  }
  else
  {
    // set the tuning capacitor for SW or MW/LW
    rx.setTuneFrequencyAntennaCapacitor((band[bandIdx].bandType == MW_BAND_TYPE || band[bandIdx].bandType == LW_BAND_TYPE) ? 0 : 1);
    if (ssbLoaded)
    {
      rx.setSSB(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq, band[bandIdx].currentFreq, tabAmStep[band[bandIdx].currentStepIdx], currentMode);
      rx.setSSBAutomaticVolumeControl(1);
      rx.setSsbSoftMuteMaxAttenuation(softMuteMaxAttIdx); // Disable Soft Mute for SSB
      bwIdxSSB = band[bandIdx].bandwidthIdx;
      rx.setSSBAudioBandwidth(bandwidthSSB[bwIdxSSB].idx);
    }
    else
    {
      currentMode = AM;
      rx.setAM(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq, band[bandIdx].currentFreq, tabAmStep[band[bandIdx].currentStepIdx]);
      bfoOn = false;
      bwIdxAM = band[bandIdx].bandwidthIdx;
      rx.setBandwidth(bandwidthAM[bwIdxAM].idx, 1);
      rx.setAmSoftMuteMaxAttenuation(softMuteMaxAttIdx); // Soft Mute for AM or SSB
    }
    rx.setAutomaticGainControl(disableAgc, agcNdx);
    rx.setSeekAmLimits(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq); // Consider the range all defined current band
    rx.setSeekAmSpacing(5); // Max 10kHz for spacing

  }
  delay(100);
  currentFrequency = band[bandIdx].currentFreq;
  currentStepIdx = band[bandIdx].currentStepIdx;

  rssi = 0;
  snr = 0;
  cleanBfoRdsInfo();
  showStatus();
}


void loadSSB() {
  rx.setI2CFastModeCustom(400000); // You can try rx.setI2CFastModeCustom(700000); or greater value
  rx.loadPatch(ssb_patch_content, size_content, bandwidthSSB[bwIdxSSB].idx);
  rx.setI2CFastModeCustom(100000);
  ssbLoaded = true; 
}

/**
 *  Switches the Bandwidth
 */
void doBandwidth(int8_t v)
{
    if (currentMode == LSB || currentMode == USB)
    {
      bwIdxSSB = (v == 1) ? bwIdxSSB + 1 : bwIdxSSB - 1;

      if (bwIdxSSB > maxSsbBw)
        bwIdxSSB = 0;
      else if (bwIdxSSB < 0)
        bwIdxSSB = maxSsbBw;

      rx.setSSBAudioBandwidth(bandwidthSSB[bwIdxSSB].idx);
      // If audio bandwidth selected is about 2 kHz or below, it is recommended to set Sideband Cutoff Filter to 0.
      if (bandwidthSSB[bwIdxSSB].idx == 0 || bandwidthSSB[bwIdxSSB].idx == 4 || bandwidthSSB[bwIdxSSB].idx == 5)
        rx.setSSBSidebandCutoffFilter(0);
      else
        rx.setSSBSidebandCutoffFilter(1);

      band[bandIdx].bandwidthIdx = bwIdxSSB;
    }
    else if (currentMode == AM)
    {
      bwIdxAM = (v == 1) ? bwIdxAM + 1 : bwIdxAM - 1;

      if (bwIdxAM > maxAmBw)
        bwIdxAM = 0;
      else if (bwIdxAM < 0)
        bwIdxAM = maxAmBw;

      rx.setBandwidth(bandwidthAM[bwIdxAM].idx, 1);
      band[bandIdx].bandwidthIdx = bwIdxAM;
      
    } else {
    bwIdxFM = (v == 1) ? bwIdxFM + 1 : bwIdxFM - 1;
    if (bwIdxFM > maxFmBw)
      bwIdxFM = 0;
    else if (bwIdxFM < 0)
      bwIdxFM = maxFmBw;

    rx.setFmBandwidth(bandwidthFM[bwIdxFM].idx);
    band[bandIdx].bandwidthIdx = bwIdxFM;
  }
  showBandwidth();
  delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
}

/**
 * Show cmd on display. It means you are setting up something.  
 */
void showCommandStatus(char * currentCmd)
{
  spr.drawString(currentCmd,38,14,2);
  drawSprite();
}

/**
 * Show menu options
 */
void showMenu() {
  drawSprite();
}

/**
 *  AGC and attenuattion setup
 */
void doAgc(int8_t v) {
  agcIdx = (v == 1) ? agcIdx + 1 : agcIdx - 1;
  if (agcIdx < 0 )
    agcIdx = 35;
  else if ( agcIdx > 35)
    agcIdx = 0;
  disableAgc = (agcIdx > 0); // if true, disable AGC; esle, AGC is enable
  if (agcIdx > 1)
    agcNdx = agcIdx - 1;
  else
    agcNdx = 0;
  rx.setAutomaticGainControl(disableAgc, agcNdx); // if agcNdx = 0, no attenuation
  showAgcAtt();
  delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
  elapsedCommand = millis();
}


/**
 * Switches the current step
 */
void doStep(int8_t v)
{
    if ( currentMode == FM ) {
      idxFmStep = (v == 1) ? idxFmStep + 1 : idxFmStep - 1;
      if (idxFmStep > lastFmStep)
        idxFmStep = 0;
      else if (idxFmStep < 0)
        idxFmStep = lastFmStep;
        
      currentStepIdx = idxFmStep;
      rx.setFrequencyStep(tabFmStep[currentStepIdx]);
      
    } else {
      idxAmStep = (v == 1) ? idxAmStep + 1 : idxAmStep - 1;
      if (idxAmStep > lastAmStep)
        idxAmStep = 0;
      else if (idxAmStep < 0)
        idxAmStep = lastAmStep;

      currentStepIdx = idxAmStep;
      rx.setFrequencyStep(tabAmStep[currentStepIdx]);
      rx.setSeekAmSpacing(5); // Max 10kHz for spacing
    }
    band[bandIdx].currentStepIdx = currentStepIdx;
    showStep();
    elapsedCommand = millis();
}

/**
 * Switches to the AM, LSB or USB modes
 */
void doMode(int8_t v)
{
  if (currentMode != FM)
  {
    if (v == 1)  { // clockwise
      if (currentMode == AM)
      {
        // If you were in AM mode, it is necessary to load SSB patch (avery time)

        spr.fillSmoothRoundRect(80,40,160,40,4,TFT_WHITE);
        spr.fillSmoothRoundRect(81,41,158,38,4,TFT_MENU_BACK);
        spr.drawString("Loading SSB",160,62,4);
        spr.pushSprite(0,0);
        
        loadSSB();
        ssbLoaded = true;
        currentMode = LSB;
      }
      else if (currentMode == LSB)
        currentMode = USB;
      else if (currentMode == USB)
      {
        currentMode = AM;
        bfoOn = ssbLoaded = false;
      }
    } else { // and counterclockwise
      if (currentMode == AM)
      {
        // If you were in AM mode, it is necessary to load SSB patch (avery time)

        spr.fillSmoothRoundRect(80,40,160,40,4,TFT_WHITE);
        spr.fillSmoothRoundRect(81,41,158,38,4,TFT_MENU_BACK);
        spr.drawString("Loading SSB",160,62,4);
        spr.pushSprite(0,0);
        
        loadSSB();
        ssbLoaded = true;
        currentMode = USB;
      }
      else if (currentMode == USB)
        currentMode = LSB;
      else if (currentMode == LSB)
      {
        currentMode = AM;
        bfoOn = ssbLoaded = false;
      }
    }
    // Nothing to do if you are in FM mode
    band[bandIdx].currentFreq = currentFrequency;
    band[bandIdx].currentStepIdx = currentStepIdx;
    useBand();
  }
  delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
  elapsedCommand = millis();
}

/**
 * Sets the audio volume
 */
void doVolume( int8_t v ) {
  if ( v == 1)
    rx.volumeUp();
  else
    rx.volumeDown();

  showVolume();
  delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
}

/**
 *  This function is called by the seek function process.
 */
void showFrequencySeek(uint16_t freq)
{
  currentFrequency = freq;
  showFrequency();
}

/**
 *  Find a station. The direction is based on the last encoder move clockwise or counterclockwise
 */
void doSeek()
{
  if ((currentMode == LSB || currentMode == USB)) return; // It does not work for SSB mode
  
  rx.seekStationProgress(showFrequencySeek, seekDirection);
  currentFrequency = rx.getFrequency();
  
}

/**
 * Sets the Soft Mute Parameter
 */
void doSoftMute(int8_t v)
{
  softMuteMaxAttIdx = (v == 1) ? softMuteMaxAttIdx + 1 : softMuteMaxAttIdx - 1;
  if (softMuteMaxAttIdx > 32)
    softMuteMaxAttIdx = 0;
  else if (softMuteMaxAttIdx < 0)
    softMuteMaxAttIdx = 32;

  rx.setAmSoftMuteMaxAttenuation(softMuteMaxAttIdx);
  showSoftMute();
  elapsedCommand = millis();
}

/**
 *  Menu options selection
 */
void doMenu( int8_t v) {
  menuIdx = (v == 1) ? menuIdx - 1 : menuIdx + 1;

  if (menuIdx > lastMenu)
    menuIdx = 0;
  else if (menuIdx < 0)
    menuIdx = lastMenu;

  showMenu();
  delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
  elapsedCommand = millis();
}


/**
 * Starts the MENU action process
 */
void doCurrentMenuCmd() {
  disableCommands();
  switch (currentMenuCmd) {
     case VOLUME:                 // VOLUME
      cmdVolume = true;
      showVolume();
      break;
    case STEP:                 // STEP
      cmdStep = true;
      showStep();
      break;
    case MODE:                 // MODE
      cmdMode = true;
      showMode();
      break;
    case BFO:
      if ((currentMode == LSB || currentMode == USB)) {
        bfoOn = true;
        showBFO();
      }
      showFrequency();
      break;      
    case BW:                 // BW
      cmdBandwidth = true;
      showBandwidth();
      break;
    case AGC_ATT:                 // AGC/ATT
      cmdAgc = true;
      showAgcAtt();
      break;
    case SOFTMUTE: 
      cmdSoftMuteMaxAtt = true;
      showSoftMute();  
      break;
    case SEEKUP:
      seekDirection = 1;
      doSeek();
      break;  
    case SEEKDOWN:
      seekDirection = 0;
      doSeek();
      break;    
    case BAND: 
      cmdBand = true;
      drawSprite();  
      break;
    case MUTE: 
      muted=!muted;
      if (muted) rx.setAudioMute(muted);
      else rx.setAudioMute(muted);
      drawSprite();  
      break;
    default:
      showStatus();
      break;
  }
  currentMenuCmd = -1;
  elapsedCommand = millis();
}

/**
 * Return true if the current status is Menu command
 */
bool isMenuMode() {
  return (cmdMenu | cmdStep | cmdBandwidth | cmdAgc | cmdVolume | cmdSoftMuteMaxAtt | cmdMode);
}

uint8_t getStrength() {
  if (currentMode != FM) {
    //dBuV to S point conversion HF
    if ((rssi >= 0) and (rssi <=  1)) return  1;  // S0
    if ((rssi >  1) and (rssi <=  1)) return  2;  // S1
    if ((rssi >  2) and (rssi <=  3)) return  3;  // S2
    if ((rssi >  3) and (rssi <=  4)) return  4;  // S3
    if ((rssi >  4) and (rssi <= 10)) return  5;  // S4
    if ((rssi > 10) and (rssi <= 16)) return  6;  // S5
    if ((rssi > 16) and (rssi <= 22)) return  7;  // S6
    if ((rssi > 22) and (rssi <= 28)) return  8;  // S7
    if ((rssi > 28) and (rssi <= 34)) return  9;  // S8
    if ((rssi > 34) and (rssi <= 44)) return 10;  // S9
    if ((rssi > 44) and (rssi <= 54)) return 11;  // S9 +10
    if ((rssi > 54) and (rssi <= 64)) return 12;  // S9 +20
    if ((rssi > 64) and (rssi <= 74)) return 13;  // S9 +30
    if ((rssi > 74) and (rssi <= 84)) return 14;  // S9 +40
    if ((rssi > 84) and (rssi <= 94)) return 15;  // S9 +50
    if  (rssi > 94)                   return 16;  // S9 +60
    if  (rssi > 95)                   return 17;  //>S9 +60
  }
  else
  {
    //dBuV to S point conversion FM
    if  (rssi <  1)                   return  1;
    if ((rssi >  1) and (rssi <=  2)) return  7;  // S6
    if ((rssi >  2) and (rssi <=  8)) return  8;  // S7
    if ((rssi >  8) and (rssi <= 14)) return  9;  // S8
    if ((rssi > 14) and (rssi <= 24)) return 10;  // S9
    if ((rssi > 24) and (rssi <= 34)) return 11;  // S9 +10
    if ((rssi > 34) and (rssi <= 44)) return 12;  // S9 +20
    if ((rssi > 44) and (rssi <= 54)) return 13;  // S9 +30
    if ((rssi > 54) and (rssi <= 64)) return 14;  // S9 +40
    if ((rssi > 64) and (rssi <= 74)) return 15;  // S9 +50
    if  (rssi > 74)                   return 16;  // S9 +60
    if  (rssi > 76)                   return 17;  //>S9 +60
    // newStereoPilot=si4735.getCurrentPilot();
  }
  return 0;
}    

void drawMenu() {
  if (cmdMenu) {
    spr.fillSmoothRoundRect(1,1,76,110,4,TFT_RED);
    spr.fillSmoothRoundRect(2,2,74,108,4,TFT_MENU_BACK);
    spr.setTextColor(TFT_WHITE,TFT_MENU_BACK);    
    spr.drawString("Menu",38,14,2);
    spr.setTextFont(0);
    spr.setTextColor(0xBEDF,TFT_MENU_BACK);
    spr.fillRoundRect(6,24+(2*16),66,16,2,0x105B);
    for(int i=-2;i<3;i++){
      if (i==0) spr.setTextColor(0xBEDF,0x105B);
      else spr.setTextColor(0xBEDF,TFT_MENU_BACK);
      spr.drawString(menu[abs((menuIdx+lastMenu+1+i)%(lastMenu+1))],38,64+(i*16),2);
    }
  } else {
    spr.setTextColor(TFT_WHITE,TFT_MENU_BACK);    
    spr.fillSmoothRoundRect(1,1,76,110,4,TFT_RED);
    spr.fillSmoothRoundRect(2,2,74,108,4,TFT_MENU_BACK);
    spr.drawString(menu[menuIdx],38,14,2);
    spr.setTextFont(0);
    spr.setTextColor(0xBEDF,TFT_MENU_BACK);
    // spr.fillRect(6,24+(2*16),67,16,0xBEDF);
    spr.fillRoundRect(6,24+(2*16),66,16,2,0x105B);
    for(int i=-2;i<3;i++){
      if (i==0) spr.setTextColor(0xBEDF,0x105B);
      else spr.setTextColor(0xBEDF,TFT_MENU_BACK);
      if (cmdMode)
        if (currentMode == FM) {
          if (i==0) spr.drawString(bandModeDesc[abs((currentMode+lastBandModeDesc+1+i)%(lastBandModeDesc+1))],38,64+(i*16),2);
        }          
        else spr.drawString(bandModeDesc[abs((currentMode+lastBandModeDesc+1+i)%(lastBandModeDesc+1))],38,64+(i*16),2);
      if (cmdStep)
        if (currentMode == FM) spr.drawNumber(tabFmStep[abs((currentStepIdx+lastFmStep+1+i)%(lastFmStep+1))],38,64+(i*16),2);
        else spr.drawNumber(tabAmStep[abs((currentStepIdx+lastAmStep+1+i)%(lastAmStep+1))],38,64+(i*16),2);
      if (cmdBand) spr.drawString(band[abs((bandIdx+lastBand+1+i)%(lastBand+1))].bandName,38,64+(i*16),2);
      if (cmdBandwidth) {
        if (currentMode == LSB || currentMode == USB)
        {
          spr.drawString(bandwidthSSB[abs((bwIdxSSB+lastBandwidthSSB+1+i)%(lastBandwidthSSB+1))].desc,38,64+(i*16),2);
          // bw = (char *)bandwidthSSB[bwIdxSSB].desc;
          // showBFO();
        }
        else if (currentMode == AM)
        {
          spr.drawString(bandwidthAM[abs((bwIdxAM+lastBandwidthAM+1+i)%(lastBandwidthAM+1))].desc,38,64+(i*16),2);
        }
        else
        {
          spr.drawString(bandwidthFM[abs((bwIdxFM+lastBandwidthFM+1+i)%(lastBandwidthFM+1))].desc,38,64+(i*16),2);
        }
      }
    }
    if (cmdVolume) {
      spr.setTextColor(0xBEDF,TFT_MENU_BACK);
      spr.fillRoundRect(6,24+(2*16),66,16,2,TFT_MENU_BACK);
      spr.drawNumber(rx.getVolume(),38,60,7);
    }
    if (cmdAgc) {
      spr.setTextColor(0xBEDF,TFT_MENU_BACK);
      spr.fillRoundRect(6,24+(2*16),66,16,2,TFT_MENU_BACK);
      rx.getAutomaticGainControl();
      if (agcNdx == 0 && agcIdx == 0) {
        spr.setFreeFont(&Orbitron_Light_24);
        spr.drawString("AGC",38,48);
        spr.drawString("On",38,72);
        spr.setTextFont(0);
      } else {
        sprintf(sAgc, "%2.2d", agcNdx);
        spr.drawString(sAgc,38,60,7);
      }
    }        
    if (cmdSoftMuteMaxAtt) {
      spr.setTextColor(0xBEDF,TFT_MENU_BACK);
      spr.fillRoundRect(6,24+(2*16),66,16,2,TFT_MENU_BACK);
      spr.drawNumber(softMuteMaxAttIdx,38,60,7);
    }
    spr.setTextColor(TFT_WHITE,TFT_BLACK);
  }
}

void drawSprite()
{
  
  spr.fillSprite(TFT_BLACK);
  spr.setTextColor(TFT_WHITE,TFT_BLACK);

  if (currentMode == FM) spr.drawFloat(currentFrequency/100.00,1,160,60,7);
  else spr.drawNumber(currentFrequency,160,60,7);

  spr.setFreeFont(&Orbitron_Light_24);
  spr.drawString(band[bandIdx].bandName,160,12);
  
  if (isMenuMode() or cmdBand) drawMenu();    
  else {
    countClick = 0;
    spr.setTextDatum(ML_DATUM);
    spr.setTextColor(TFT_WHITE,TFT_MENU_BACK);    
    spr.fillSmoothRoundRect(1,1,76,110,4,TFT_WHITE);
    spr.fillSmoothRoundRect(2,2,74,108,4,TFT_MENU_BACK);
    spr.drawString("Band:",6,64+(-3*16),2);    
    spr.drawString(band[bandIdx].bandName,48,64+(-3*16),2);
    spr.drawString("Mode:",6,64+(-2*16),2);    
    spr.drawString(bandModeDesc[currentMode],48,64+(-2*16),2);    
    spr.drawString("Step:",6,64+(-1*16),2);    
    if (currentMode == FM) spr.drawNumber(tabFmStep[currentStepIdx],48,64+(-1*16),2);
    else spr.drawNumber(tabAmStep[currentStepIdx],48,64+(-1*16),2);
    spr.drawString("BW:",6,64+(0*16),2);        
    if (currentMode == LSB || currentMode == USB)
    {
      spr.drawString(bandwidthSSB[bwIdxSSB].desc,48,64+(0*16),2);
    }
    else if (currentMode == AM)
    {
      spr.drawString(bandwidthAM[bwIdxAM].desc,48,64+(0*16),2);
    }
    else
    {
      spr.drawString(bandwidthFM[bwIdxFM].desc,48,64+(0*16),2);
    }
    if (agcNdx == 0 && agcIdx == 0) {
      spr.drawString("AGC:",6,64+(1*16),2);        
      spr.drawString("On",48,64+(1*16),2);
    } else {
      sprintf(sAgc, "%2.2d", agcNdx);
      spr.drawString("Att:",6,64+(1*16),2);        
      spr.drawString(sAgc,48,64+(1*16),2);
    }
    spr.drawString("BFO:",6,64+(2*16),2);
    if (currentMode == LSB || currentMode == USB) {
      spr.setTextDatum(MR_DATUM);
      spr.drawString(bfo,74,64+(2*16),2);
    }
    else spr.drawString("Off",48,64+(2*16),2);
    spr.setTextDatum(MC_DATUM);
  }

  if (bfoOn) {
    spr.setTextColor(TFT_WHITE,TFT_BLACK);
    spr.drawString("BFO:",125,102,4);
    spr.setTextDatum(MR_DATUM);
    spr.drawString(bfo,225,102,4);
    spr.setTextDatum(MC_DATUM);    
  
  }

  spr.setTextFont(0);
  spr.setTextColor(TFT_WHITE,TFT_BLACK);
  
  spr.drawString("SIGNAL:",266,54);
  if (muted) {
      spr.setTextColor(TFT_RED,TFT_BLACK);
      spr.drawString("MUTE ON",272,102,2);
      spr.setTextColor(TFT_WHITE,TFT_BLACK);
  }
  else {
      spr.drawString("VOL:",257,102,2);    
      spr.drawNumber(rx.getVolume(),282,102,2);
  }
  
  for(int i=0;i<getStrength();i++)
    if (i<10)
      spr.fillRect(244+(i*4),80-(i*1),2,4+(i*1),0x3526);
    else
      spr.fillRect(244+(i*4),80-(i*1),2,4+(i*1),TFT_RED);
  
  
  spr.fillTriangle(156,112,160,122,164,112,TFT_RED);
  spr.drawLine(160,114,160,170,TFT_RED);

  int temp=(currentFrequency/10.00)-20;
  uint16_t lineColor;
  for(int i=0;i<40;i++)
  {
    if (i==20) lineColor=TFT_RED;
    else lineColor=0xC638;
    if (!(temp<band[bandIdx].minimumFreq/10.00 or temp>band[bandIdx].maximumFreq/10.00)) {
      if((temp%10)==0){
        spr.drawLine(i*8,170,i*8,140,lineColor);
        spr.drawLine((i*8)+1,170,(i*8)+1,140,lineColor);
        if (currentMode == FM) spr.drawFloat(temp/10.0,1,i*8,130,2);
        else if (temp >= 100) spr.drawFloat(temp/100.0,3,i*8,130,2);
               else spr.drawNumber(temp*10,i*8,130,2);
      } else if((temp%5)==0 && (temp%10)!=0) {
        spr.drawLine(i*8,170,i*8,150,lineColor);
        spr.drawLine((i*8)+1,170,(i*8)+1,150,lineColor);
        // spr.drawFloat(temp/10.0,1,i*8,144);        
      } else {
        spr.drawLine(i*8,170,i*8,160,lineColor);
      }
    }
  
   temp=temp+1;
  }

  if (currentMode == FM) {
    spr.fillSmoothRoundRect(240,20,76,22,4,TFT_WHITE);
    spr.fillSmoothRoundRect(241,21,74,20,4,TFT_BLACK);
    if (rx.getCurrentPilot()) {
      spr.setTextColor(TFT_RED,TFT_BLACK);
      spr.drawString("FM Stereo",278,31,2);
      spr.setTextColor(TFT_WHITE,TFT_BLACK);
    } else spr.drawString("FM Mono",278,31,2);
  } else {
    spr.fillSmoothRoundRect(240,20,76,22,4,TFT_WHITE);
    spr.fillSmoothRoundRect(241,21,74,20,4,TFT_BLACK);
    spr.drawString(bandModeDesc[currentMode],278,31,2);
  }
   
  
  // spr.setTextColor(TFT_MAGENTA,TFT_BLACK);
  spr.drawString(bufferStationName,160,102,4);
  // spr.setTextColor(TFT_WHITE,TFT_BLACK);

  batteryMonitor(true);   
  spr.pushSprite(0,0);
}

void cleanBfoRdsInfo()
{
  bufferStationName[0]='\0';
}

void showRDSMsg()
{
  rdsMsg[35] = bufferRdsMsg[35] = '\0';
  if (strcmp(bufferRdsMsg, rdsMsg) == 0)
    return;
}

void showRDSStation()
{
  if (strcmp(bufferStationName, stationName) == 0 ) return;
  cleanBfoRdsInfo();
  strcpy(bufferStationName, stationName);
  drawSprite();
}

void showRDSTime()
{

  if (strcmp(bufferRdsTime, rdsTime) == 0)
    return;
}

void checkRDS()
{
  rx.getRdsStatus();
  if (rx.getRdsReceived())
  {
    if (rx.getRdsSync() && rx.getRdsSyncFound())
    {
      rdsMsg = rx.getRdsText2A();
      stationName = rx.getRdsText0A();
      rdsTime = rx.getRdsTime();
      // if ( rdsMsg != NULL )   showRDSMsg();
      if (stationName != NULL)         
          showRDSStation();
      // if ( rdsTime != NULL ) showRDSTime();
    }
  }
}

/***************************************************************************************
** Function name:           DrawBatteryLevel
** Description:             Draw a battery level icon
***************************************************************************************/
// Draw a battery level icon
void DrawBatteryLevel(int batteryLevel) {

  int chargeLevel;
  uint16_t batteryLevelColor;

  if ( batteryLevel == 1 ) {
    chargeLevel=4;
    batteryLevelColor=TFT_RED;
  }
  if ( batteryLevel == 2 ) {
    chargeLevel=9;
    batteryLevelColor=TFT_YELLOW;
  }
  if ( batteryLevel == 3 ) {
    chargeLevel=13;
    batteryLevelColor=TFT_YELLOW;
  }
  if ( batteryLevel == 4 ) {
    chargeLevel=18;
    batteryLevelColor=TFT_GREEN;
  }
  if ( batteryLevel == 5 ) {   // To do: Animated icon to charge mode!
    chargeLevel=18;
    batteryLevelColor=TFT_GREEN;
  }
  spr.drawRect(XbatPos, YbatPos, Xbatsiz, Ybatsiz, TFT_WHITE);
  spr.fillRect(XbatPos+1, YbatPos+1, Xbatsiz-2, Ybatsiz-2, TFT_BLACK);
  spr.fillRect(XbatPos+1, YbatPos+1, chargeLevel, Ybatsiz-2, batteryLevelColor);
  spr.fillRect(XbatPos+20, YbatPos+2, 2, 5, TFT_WHITE);
  spr.pushSprite(0,0);

}

/***************************************************************************************
** Function name:           batteryMonitor
** Description:             Check Battery Level
***************************************************************************************/
// Check Battery Level
void batteryMonitor(bool forced) {
  if(battery.getBatteryVolts() >= MIN_USB_VOLTAGE){
    if (!batteryCharging or forced) {
      batteryCharging = true;
      DrawBatteryLevel(5);
    }
  } else {
      batteryCharging = false;
      int batteryLevel = battery.getBatteryChargeLevel();
      if(batteryLevel >=80){
        currentBatteryLevel = 4;
      }else if(batteryLevel < 80 && batteryLevel >= 50 ){
        currentBatteryLevel = 3;
      }else if(batteryLevel < 50 && batteryLevel >= 20 ){
        currentBatteryLevel = 2;
      }else if(batteryLevel < 20 ){
        currentBatteryLevel = 1;
      }  
      if (currentBatteryLevel != previousBatteryLevel or forced){
      DrawBatteryLevel(currentBatteryLevel);
      previousBatteryLevel = currentBatteryLevel;
      }
  }
  yield();  
}

/**
 * Main loop
 */
void loop()
{

  batteryMonitor(false);  // Battery check

  // Check if the encoder has moved.
  if (encoderCount != 0)
  {
    if (bfoOn & (currentMode == LSB || currentMode == USB))
    {
      currentBFO = (encoderCount == 1) ? (currentBFO + currentBFOStep) : (currentBFO - currentBFOStep);
      rx.setSSBBfo(currentBFO);
      showBFO();
    }
    else if (cmdMenu)
      doMenu(encoderCount);
    else if (cmdMode)
      doMode(encoderCount);
    else if (cmdStep)
      doStep(encoderCount);
    else if (cmdAgc)
      doAgc(encoderCount);
    else if (cmdBandwidth)
      doBandwidth(encoderCount);
    else if (cmdVolume)
      doVolume(encoderCount);
    else if (cmdSoftMuteMaxAtt)
      doSoftMute(encoderCount);
    else if (cmdBand)
      setBand(encoderCount);
    else
    {
      if (encoderCount == 1)
      {
        rx.frequencyUp();
      }
      else
      {
        rx.frequencyDown();
      }
      if (currentMode == FM) cleanBfoRdsInfo();
      // Show the current frequency only if it has changed
      currentFrequency = rx.getFrequency();
      showFrequency();
    }
    encoderCount = 0;
    resetEepromDelay();
    delay(MIN_ELAPSED_TIME);
    elapsedCommand = millis();
  }
  else
  {
    if (digitalRead(ENCODER_PUSH_BUTTON) == LOW)
    {
       uint32_t timestamp = millis() + 3000;
        while (digitalRead(ENCODER_PUSH_BUTTON) == LOW ) { 
          if( millis() > timestamp){
            tft.fillScreen(TFT_BLACK);
            tft.setTextSize(2);
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.setTextDatum(MC_DATUM);
            tft.setCursor(tft.width()/2,tft.height()/2);
            tft.println("SLEEP");
            delay(3000);
            digitalWrite(PIN_POWER_ON, 0);
            esp_sleep_enable_ext0_wakeup((gpio_num_t)ENCODER_PUSH_BUTTON, 0);
            esp_deep_sleep_start();
          }
      }
      timestamp = 0;
      countClick++;
      if (cmdMenu)
      {
        currentMenuCmd = menuIdx;
        doCurrentMenuCmd();
      }
      else if (countClick == 1)
      { // If just one click, you can select the band by rotating the encoder
        if (isMenuMode())
        {
          disableCommands();
          showStatus();
          showCommandStatus((char *)"VFO ");
        }
        else if (bfoOn) {
          bfoOn = false;
          showStatus();
        }
        else
        {
          cmdBand = !cmdBand;
          // cmdMenu = !cmdMenu;
          menuIdx=BAND;          
          currentMenuCmd = menuIdx;
          drawSprite();
        }
      }
      else
      { // GO to MENU if more than one click in less than 1/2 seconds.
        cmdMenu = !cmdMenu;
        if (cmdMenu)
          showMenu();
      }
      delay(MIN_ELAPSED_TIME);
      elapsedCommand = millis();
    }
  }

  // Show RSSI status only if this condition has changed
  if ((millis() - elapsedRSSI) > MIN_ELAPSED_RSSI_TIME * 6)
  {
    rx.getCurrentReceivedSignalQuality();
    snr= rx.getCurrentSNR();
    int aux = rx.getCurrentRSSI();
    if (rssi != aux && !isMenuMode())
    {
      rssi = aux;
      showRSSI();
    }
    elapsedRSSI = millis();
  }

  // Disable commands control
  if ((millis() - elapsedCommand) > ELAPSED_COMMAND)
  {
    if ((currentMode == LSB || currentMode == USB) )
    {
      bfoOn = false;
      // showBFO();
      disableCommands();
      showStatus();
    } else if (isMenuMode() or cmdBand) {
      disableCommands();
      showStatus();
    } 
    elapsedCommand = millis();
  }

  if ((millis() - elapsedClick) > ELAPSED_CLICK)
  {
    countClick = 0;
    elapsedClick = millis();
  }

  if ((millis() - lastRDSCheck) > RDS_CHECK_TIME) {
    if ((currentMode == FM) and (snr >= 12)) checkRDS();
    lastRDSCheck = millis();
  }  

  // Show the current frequency only if it has changed
  if (itIsTimeToSave)
  {
    if ((millis() - storeTime) > STORE_TIME)
    {
      saveAllReceiverInformation();
      storeTime = millis();
      itIsTimeToSave = false;
    }
  }

  delay(5);
}
