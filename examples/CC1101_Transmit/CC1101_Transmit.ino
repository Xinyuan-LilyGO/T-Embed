/**
 * @file      CC1101_Transmit.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2023-10-19
 * @note      The sketch only works with CC1101 + NFC Shield
 */
#include <RadioLib.h>
#include <Adafruit_PN532.h>
#include "TFTHelper.h"


static float freq = 0;

enum CC1101_Freq  {
    FREQ_315MHZ,
    FREQ_434MHZ,
    FREQ_868MHZ,
    FREQ_915MHZ,
};

SPIClass newSPI =  SPIClass(HSPI);
CC1101 radio = new Module(SHIELD_RADIO_CS_PIN,
                          SHIELD_RADIO_DIO0_PIN,
                          SHIELD_RADIO_RST_PIN,
                          -1,
                          newSPI);
Adafruit_PN532 nfc(SHIELD_NFC_CS_PIN, &newSPI);

TFT_eSprite spr = TFT_eSprite(&tft);
TFT_eSprite sprTitle = TFT_eSprite(&tft);
TFT_eSprite nfcSpr = TFT_eSprite(&tft);

int spr_w = 0;
int spr_h = 0;
int spr_start_x = 0;
int spr_start_y = 0;
int start_x ;
int title_h ;

void readMifareClassic(void);

void setFreq( CC1101_Freq f)
{
    switch (f) {
    case FREQ_315MHZ:
        digitalWrite(SHIELD_RADIO_SW1_PIN, HIGH);
        digitalWrite(SHIELD_RADIO_SW0_PIN, LOW);
        freq = 315.0;
        return;
    case FREQ_434MHZ:
        digitalWrite(SHIELD_RADIO_SW1_PIN, HIGH);
        digitalWrite(SHIELD_RADIO_SW0_PIN, HIGH);
        freq = 434.0;
        return;
    case FREQ_868MHZ:
        freq = 868.0;
        digitalWrite(SHIELD_RADIO_SW1_PIN, LOW);
        digitalWrite(SHIELD_RADIO_SW0_PIN, HIGH);
        return;
    case FREQ_915MHZ:
        freq = 915.0;
        digitalWrite(SHIELD_RADIO_SW1_PIN, LOW);
        digitalWrite(SHIELD_RADIO_SW0_PIN, HIGH);
        return;
    default:
        break;
    }
    freq = 868.0;
    digitalWrite(SHIELD_RADIO_SW1_PIN, LOW);
    digitalWrite(SHIELD_RADIO_SW0_PIN, HIGH);
}


void setup()
{
    Serial.begin(115200);

    // Using battery requires setting IO46 to HIGH,
    // otherwise the device will not allow
    pinMode(BOARD_POWER_ON_PIN, OUTPUT);
    digitalWrite(BOARD_POWER_ON_PIN, HIGH);

    // Initialize display screen
    initTFT();

    spr_w = tft.width();
    spr_h = tft.width() / 3;

    // Radio and NFC ,SD share the SPI bus.
    // Before using it, turn off and disable all devices
    // on the same bus.
    pinMode(SHIELD_RADIO_CS_PIN, OUTPUT);
    digitalWrite(SHIELD_RADIO_CS_PIN, HIGH);

    pinMode(SHIELD_NFC_CS_PIN, OUTPUT);
    digitalWrite(SHIELD_NFC_CS_PIN, HIGH);

    pinMode(BOARD_SDCARD_CS_PIN, OUTPUT);
    digitalWrite(BOARD_SDCARD_CS_PIN, HIGH);

    //Set antenna frequency settings
    pinMode(SHIELD_RADIO_SW1_PIN, OUTPUT);
    pinMode(SHIELD_RADIO_SW0_PIN, OUTPUT);
    //Set CC1101 frequency
    setFreq(FREQ_868MHZ);

    // Initialize SPI
    newSPI.begin(BOARD_SPI_SCK_PIN, BOARD_SPI_MISO_PIN, BOARD_SPI_MOSI_PIN);


    nfc.begin();
    uint32_t versiondata = nfc.getFirmwareVersion();
    if (! versiondata) {
        Serial.print("Didn't find PN53x chip");
        tft.print("Didn't find PN53x chip");
        while (1); // halt
    }

    // Got ok data, print it out!
    Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
    Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC);
    Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);
    Serial.println("Waiting for an ISO14443A Card ...");


    // initialize CC1101
    Serial.print(F("[CC1101] Initializing ... "));
    Serial.print(freq);
    Serial.print(" MHz ");

    tft.print(F("[CC1101] Initializing ... "));
    tft.print(freq);
    tft.print(" MHz ");

    int state = radio.begin(freq);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
        tft.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        tft.print(F("failed, code "));
        tft.println(state);
        while (true);
    }


    // you can also change the settings at runtime
    // and check if the configuration was changed successfully

    // set carrier frequency
    if (radio.setFrequency(freq) == RADIOLIB_ERR_INVALID_FREQUENCY) {
        Serial.println(F("[CC1101] Selected frequency is invalid for this module!"));
        tft.println(F("[CC1101] Selected frequency is invalid for this module!"));
        while (true);
    }

    // Too high an air rate may result in data loss
    // Set bit rate to 5 kbps
    state = radio.setBitRate(5);
    if (state == RADIOLIB_ERR_INVALID_BIT_RATE) {
        Serial.println(F("[CC1101] Selected bit rate is invalid for this module!"));
        tft.println(F("[CC1101] Selected bit rate is invalid for this module!"));
        while (true);
    } else if (state == RADIOLIB_ERR_INVALID_BIT_RATE_BW_RATIO) {
        Serial.println(F("[CC1101] Selected bit rate to bandwidth ratio is invalid!"));
        Serial.println(F("[CC1101] Increase receiver bandwidth to set this bit rate."));
        tft.println(F("[CC1101] Increase receiver bandwidth to set this bit rate."));
        while (true);
    }

    // set receiver bandwidth to 135.0 kHz
    if (radio.setRxBandwidth(135.0) == RADIOLIB_ERR_INVALID_RX_BANDWIDTH) {
        Serial.println(F("[CC1101] Selected receiver bandwidth is invalid for this module!"));
        tft.println(F("[CC1101] Selected receiver bandwidth is invalid for this module!"));
        while (true);
    }

    // set allowed frequency deviation to 10.0 kHz
    if (radio.setFrequencyDeviation(10.0) == RADIOLIB_ERR_INVALID_FREQUENCY_DEVIATION) {
        Serial.println(F("[CC1101] Selected frequency deviation is invalid for this module!"));
        tft.println(F("[CC1101] Selected frequency deviation is invalid for this module!"));
        while (true);
    }

    // set output power to 10 dBm
    if (radio.setOutputPower(10) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
        Serial.println(F("[CC1101] Selected output power is invalid for this module!"));
        tft.println(F("[CC1101] Selected output power is invalid for this module!"));
        while (true);
    }

    // 2 bytes can be set as sync word
    if (radio.setSyncWord(0x01, 0x23) == RADIOLIB_ERR_INVALID_SYNC_WORD) {
        Serial.println(F("[CC1101] Selected sync word is invalid for this module!"));
        tft.println(F("[CC1101] Selected sync word is invalid for this module!"));
        while (true);
    }


    start_x = (tft.width() / 2) - 2;
    title_h = tft.height() - spr_h - 10;

    sprTitle.createSprite(tft.width() / 2, title_h);
    sprTitle.fillSprite(TFT_BLACK);
    sprTitle.drawRoundRect(0, 0, start_x, title_h, 5, TFT_VIOLET);

    String str = String(freq);
    str.concat("MHz");

    sprTitle.setFreeFont(&FreeMono9pt7b);
    sprTitle.drawString("Sender:", 15, 5);
    sprTitle.drawString(str, 15, 25);
    sprTitle.pushSprite(0, 0);


    nfcSpr.createSprite(tft.width() / 2, title_h);
    nfcSpr.fillSprite(TFT_BLACK);
    nfcSpr.setFreeFont(&FreeMono9pt7b);
    nfcSpr.drawRoundRect(0, 0, start_x, title_h, 5, TFT_DARKCYAN);
    str = "NFC";
    nfcSpr.drawString(str,  15, 5);
    nfcSpr.pushSprite(start_x, 0);


    spr_start_x = 10;
    spr_start_y = 35;
    spr.createSprite(spr_w, spr_h);
    spr.fillSprite(TFT_BLACK);
    spr.drawRoundRect(0, 0, spr_w - 2, spr_h - 2, 5, TFT_ORANGE);
    spr.setCursor(spr_start_x, spr_start_y);
    spr.setFreeFont(&FreeSerif9pt7b);
    spr.pushSprite(0, tft.height() / 3);
}

void loop()
{
    // Block for 2 seconds to read the tag
    readMifareClassic();

    spr.fillSprite(TFT_BLACK);
    spr.drawRoundRect(0, 0, spr_w - 2, spr_h - 2, 5, TFT_ORANGE);
    spr.setCursor(0, spr_start_y);

    Serial.print("[");
    Serial.print(millis() / 1000);
    Serial.print("][CC1101] Transmitting packet");

    spr.print("[");
    spr.print(millis() / 1000);
    spr.println("]");
    spr.println("[CC1101] Transmitting packet");


    // Add Embed: as the header of the sending and receiving strings
    // you can transmit C-string or Arduino string up to 63 characters long
    String msg = "Embed:";
    msg.concat("I'am CC1101 [");
    msg.concat(String(millis() / 1000));
    msg.concat("]");

    int state = radio.transmit(msg);

    // you can also transmit byte array up to 63 bytes long
    /*
      byte byteArr[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
      int state = radio.transmit(byteArr, 8);
    */

    if (state == RADIOLIB_ERR_NONE) {
        // the packet was successfully transmitted
        Serial.println(F("success!"));
        spr.println(F(" success!"));

    } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
        // the supplied packet was longer than 64 bytes
        Serial.println(F("too long!"));
        spr.println(F("too long!"));

    } else {
        // some other error occurred
        Serial.print(F("failed, code "));
        Serial.println(state);
        spr.print(F("failed, code "));
        spr.println(state);
    }

    spr.pushSprite(0, tft.height() / 3);


    // // wait for a second before transmitting again
    // delay(1000);
}


void readMifareClassic(void)
{
    uint8_t success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

    // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
    // 'uid' will be populated with the UID, and uidLength will indicate
    // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 2000);
    Serial.print("readPassiveTargetID:");
    Serial.println(success);
    if (success) {
        // Display some basic information about the card
        Serial.println("Found an ISO14443A card");
        Serial.print("  UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
        Serial.print("  UID Value: ");
        nfc.PrintHex(uid, uidLength);


        //Show uid
        nfcSpr.fillSprite(TFT_BLACK);
        nfcSpr.drawRoundRect( 2, 0, start_x, title_h, 5, TFT_DARKCYAN);
        nfcSpr.setFreeFont(&FreeMono9pt7b);

        String str = "NFC";
        nfcSpr.drawString(str,  15, 5);
        nfcSpr.setTextFont(1);
        str = "UID:";
        str.concat(uid[0]); str.concat(':');
        str.concat(uid[1]); str.concat(':');
        str.concat(uid[2]); str.concat(':');
        str.concat(uid[3]);
        nfcSpr.drawString(str, 15, 25);

        nfcSpr.pushSprite(start_x, 0);

        if (uidLength == 4) {
            // We probably have a Mifare Classic card ...
            uint32_t cardid = uid[0];
            cardid <<= 8;
            cardid |= uid[1];
            cardid <<= 8;
            cardid |= uid[2];
            cardid <<= 8;
            cardid |= uid[3];
            Serial.print("Seems to be a Mifare Classic card #");
            Serial.println(cardid);
        }
        Serial.println("");
    } else {
        nfcSpr.fillSprite(TFT_BLACK);
        String str = "NFC";
        nfcSpr.setFreeFont(&FreeMono9pt7b);
        nfcSpr.drawRoundRect( 2, 0, start_x, title_h, 5, TFT_DARKCYAN);
        nfcSpr.drawString(str,  15, 5);
        nfcSpr.pushSprite(start_x, 0);
    }
}