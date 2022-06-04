#include <Arduino.h>
#include <U8g2lib.h>
#include <ArtnetWifi.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

ArtnetWiFiReceiver artnet;
const int startUniverse = 0;
const int maxUniverses = 2;
bool universesReceived[maxUniverses];
bool sendFrame = 1;

uint32_t id = 0;
char name[64];
char sname[18];

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* clock=*/16, /* data=*/17); // ESP32 Thing, HW I2C with pin remapping

unsigned char displaybuffer[1024];

void twolinetext(char *t1, char *t2)
{
  u8g2.clearBuffer();
  u8g2.drawStr(0, 28, t1);
  u8g2.drawStr(0, 60, t2);
  u8g2.sendBuffer();
};

void onDmxFrame(uint32_t universe, const uint8_t *data, const uint16_t size)
{
  sendFrame = 1;
  // set brightness of the whole strip
  if (universe == 15)
  {
    u8g2.setContrast(data[0]);
  }

  // range check
  if (universe < startUniverse)
  {
    return;
  }
  uint8_t index = universe - startUniverse;
  if (index >= maxUniverses)
  {
  }

  // Store which universe has got in
  universesReceived[index] = true;

  for (int i = 0; i < maxUniverses; i++)
  {
    if (!universesReceived[i])
    {
      sendFrame = 0;
      break;
    }
  }

  // read universe and put into the right part of the display buffer
  
  memcpy(&displaybuffer[universe*512], data, 512);

  if (sendFrame)
  {
    u8g2.clearBuffer();
  u8g2.drawBitmap(0,0,16,64,displaybuffer);
  u8g2.sendBuffer();
    // Reset universeReceived to 0
    memset(universesReceived, 0, maxUniverses);
  }
}

void setup(void)
{
  WiFi.mode(WIFI_STA);
  Serial.begin(115200);
  
  for (int i = 0; i < 17; i = i + 8)
  {
    id |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  sprintf(name, "ARTNET_OLED_%08X", id);
  sprintf(sname, "AO%08X", id);
  u8g2.begin();
  u8g2.setFontMode(0); // enable transparent mode, which is faster
  u8g2.setFont(u8g2_font_12x6LED_tf);
  twolinetext("starting", sname);

  WiFiManager wm;
  wm.setHostname(name);
  bool res;
  res = wm.autoConnect(name);
  if (!res)
  {
    twolinetext((char *)"Failed", (char *)"to connect");
    ESP.restart();
  }
  char full_text[64];
  sprintf(full_text, "IP %s", sname);

  twolinetext((char *)full_text, (char *)WiFi.localIP().toString().c_str());
  artnet.shortname(sname);
  artnet.longname(name);
  artnet.begin();
  memset(universesReceived, 0, maxUniverses);
  artnet.subscribe(onDmxFrame);
}

void loop(void)
{
  artnet.parse();
}
