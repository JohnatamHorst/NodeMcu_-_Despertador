/*
*****************************************************************************************
*AUTOR: JOHNATAM RENAN HORST
*DATA 24/12/2018
*DESCRISAO:
* Codigo Criado para funcionar como um Despertador, utilizando um display TFT para nostragem 
* das horas(obitada via Server NTP), e com um piezoelectric para gerar o alarme!!
* 
*
*
*****************************************************************************************
 */
#include <TimeLib.h> 
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Ticker.h>
#include <SPI.h>
#include <TFT_eSPI.h> 

#define TFT_GREY 0x5AEB

Ticker            timerSec;
//VARIAVEIS DISPLAY
TFT_eSPI          tft           = TFT_eSPI();       // Invoke custom library
float             sx            = 0, 
                  sy            = 1, 
                  mx            = 1, 
                  my            = 0, 
                  hx            = -1, 
                  hy            = 0;    // Saved H, M, S x & y multipliers
float             sdeg          = 0, 
                  mdeg          = 0, 
                  hdeg          = 0;
uint16_t          osx           = 120, 
                  osy           = 120, 
                  omx           = 120, 
                  omy           = 120, 
                  ohx           = 120, 
                  ohy           = 120;  // Saved H, M, S x & y coords
uint16_t          x0            = 0, 
                  x1            = 0, 
                  yy0           = 0, 
                  yy1           = 0;
boolean           initial       = 1;     
uint8_t           hh            = 0, 
                  mm            = 0, 
                  ss            = 0;
//VARIAVEIS WIRELESS
const char*       ssid          = "wifi_Johnatan";   
const char*       pwd           = "99434266";
//VARIAVEIS PROTOCOLO UDP
WiFiUDP           udp;
unsigned int      localPort     = 8888;
//VARIAVEIS SERVER NTP
IPAddress         timeServer    (132, 163, 4, 101);
const int         timeZone      = -2;                //ver horaio de Verão
//VARIAVEIS DESPERTADOR
unsigned int      timeUp[]      = {11,30};       
boolean           desativate    = true;  
//FUNÇÕES METODOS         
static uint8_t    conv2d(const char* p);   
void              initDisplay();
void              everySeconds();
void              desativateInterrupt();  
void              sendNTPpacket(IPAddress &address);
time_t            getNtpTime();
                  
void setup(void){
  pinMode(D1,OUTPUT);
  timerSec.attach(1,everySeconds);
  attachInterrupt(D2,desativateInterrupt,RISING);
  initDisplay();
  Serial.begin(115200);
  Serial.println("**********DESPERTADOR***********");
  WiFi.begin(ssid,pwd);
  while(WiFi.status() != WL_CONNECTED){
    Serial.println(".");
    delay(500);
  }
  Serial.println("Connecting to NTP Server");
  udp.begin(localPort);
  setSyncProvider(getNtpTime);
  hh  = hour();
  mm  = minute();
  ss  = second();
  Serial.println(hour());

}

void loop(){

  if(timeUp[0] == hour() && timeUp[1] == minute() && desativate){
    analogWrite(D1,50);  
  }else{
    analogWrite(D1,0);
  }
  
}







void desativateInterrupt(){
  desativate = false;
}


static uint8_t conv2d(const char* p) {
  uint8_t v = 0;
  if ('0' <= *p && *p <= '9')
    v = *p - '0';
  return 10 * v + *++p - '0';
}
void initDisplay(){
  tft.init();
  tft.setRotation(2);
  tft.fillScreen(TFT_GREY);  
  tft.setTextColor(TFT_WHITE, TFT_GREY);  // Adding a background colour erases previous text automatically
  tft.fillCircle(120, 120, 118, TFT_GREEN);
  tft.fillCircle(120, 120, 110, TFT_BLACK);

  // Draw 12 lines
  for(int i = 0; i<360; i+= 30) {
    sx = cos((i-90)*0.0174532925);
    sy = sin((i-90)*0.0174532925);
    x0 = sx*114+120;
    yy0 = sy*114+120;
    x1 = sx*100+120;
    yy1 = sy*100+120;
    tft.drawLine(x0, yy0, x1, yy1, TFT_GREEN);
  }

  // Draw 60 dots
  for(int i = 0; i<360; i+= 6) {
    sx = cos((i-90)*0.0174532925);
    sy = sin((i-90)*0.0174532925);
    x0 = sx*102+120;
    yy0 = sy*102+120;
    // Draw minute markers
    tft.drawPixel(x0, yy0, TFT_WHITE);    
    // Draw main quadrant dots
    if(i==0 || i==180) tft.fillCircle(x0, yy0, 2, TFT_WHITE);
    if(i==90 || i==270) tft.fillCircle(x0, yy0, 2, TFT_WHITE);
  }

  tft.fillCircle(120, 121, 3, TFT_WHITE);

  // Draw text at position 120,260 using fonts 4
  // Only font numbers 2,4,6,7 are valid. Font 6 only contains characters [space] 0 1 2 3 4 5 6 7 8 9 : . - a p m
  // Font 7 is a 7 segment font and only contains characters [space] 0 1 2 3 4 5 6 7 8 9 : .
 
}
void everySeconds(){

  ss++;              // Advance second
    if (ss==60) {
      ss=0;
      mm++;            // Advance minute
      if(mm>59) {
        mm=0;
        hh++;          // Advance hour
        if (hh>23) {
          hh=0;
        }
      }
    }

    // Pre-compute hand degrees, x & y coords for a fast screen update
    sdeg = ss*6;                  // 0-59 -> 0-354
    mdeg = mm*6+sdeg*0.01666667;  // 0-59 -> 0-360 - includes seconds
    hdeg = hh*30+mdeg*0.0833333;  // 0-11 -> 0-360 - includes minutes and seconds
    hx = cos((hdeg-90)*0.0174532925);    
    hy = sin((hdeg-90)*0.0174532925);
    mx = cos((mdeg-90)*0.0174532925);    
    my = sin((mdeg-90)*0.0174532925);
    sx = cos((sdeg-90)*0.0174532925);    
    sy = sin((sdeg-90)*0.0174532925);

    if (ss==0 || initial) {
      initial = 0;
      // Erase hour and minute hand positions every minute
      tft.drawLine(ohx, ohy, 120, 121, TFT_BLACK);
      ohx = hx*62+121;    
      ohy = hy*62+121;
      tft.drawLine(omx, omy, 120, 121, TFT_BLACK);
      omx = mx*84+120;    
      omy = my*84+121;
    }

      // Redraw new hand positions, hour and minute hands not erased here to avoid flicker
      tft.drawLine(osx, osy, 120, 121, TFT_BLACK);
      osx = sx*90+121;    
      osy = sy*90+121;
      tft.drawLine(osx, osy, 120, 121, TFT_RED);
      tft.drawLine(ohx, ohy, 120, 121, TFT_WHITE);
      tft.drawLine(omx, omy, 120, 121, TFT_WHITE);
      tft.drawLine(osx, osy, 120, 121, TFT_RED);

    tft.fillCircle(120, 121, 3, TFT_RED);

}
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  while (udp.parsePacket() > 0);
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      // nesse bytes(40,41,42,43) tem a informação de segundos desde 1900, isso para a time Zone = 0
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; 
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address){
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  udp.beginPacket("pool.ntp.br", 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

