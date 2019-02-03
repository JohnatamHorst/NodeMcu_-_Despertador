/*
*****************************************************************************************
*AUTOR: JOHNATAM RENAN HORST
*DATA 24/12/2018
*DESCRISAO:
* Codigo Criado para funcionar como um Despertador, utilizando um display TFT para nostragem 
* das horas(obitada via Server NTP), e com um piezoelectric para gerar o alarme!!
*LIGAÇÕES:
*
*D1 É LIGADO O BOTÃO DE DESLIGAR O ALARME(LIGAR SABENDO QUE FOI CONFIGURADO COM PULLUP INTERNO)
*D2 É A SAIDA PARA P PIEZOELETRIC
*
*ESP          ILI9341
*3.3V         (1)VCC 
*GND          (2)GND
*GND          (3)CS
*RESET        (4)RESET
*D8           (5)DC
*D7           (6)SDI/MOSI
*D5           (7)SCK
*3.3V         (8)LED
*             (9)"NÃO LIGADO, PINO DO ILI9341 FICA FLUTUANDO"           
*       
*       
*       
*****************************************************************************************
 */
#include <TimeLib.h> 
#include <FS.h>
#include <ESP8266WiFi.h>
#include <NTP.h>
#include <Ticker.h>
#include <SPI.h>
#include <TFT_eSPI.h> 
#include <ESP8266WebServer.h>


#define TFT_GREY 0x5AEB


File              file;
Dir               dir;
String            music_1       = "/alarme.wav";
Ticker            timerSec;
Ticker            despertar;
Ticker            verifTime;
ESP8266WebServer  server(80);
//VARIAVEIS DISPLAY
TFT_eSPI          tft           = TFT_eSPI();       
float             sx            = 0, 
                  sy            = 1, 
                  mx            = 1, 
                  my            = 0, 
                  hx            = -1, 
                  hy            = 0;   
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
const char*       ssidClient    = "wifi_Johnatan";   
const char*       pwdClient     = "99434266";
const char*       ssidAP        = "Despertador";   
const char*       pwdAP         = "despertador";
//VARIAVEIS DESPERTADOR
unsigned int      timeUp[]      = {11,30};       
boolean           desativate    = true; 
//VARIAVEIS SERVER ACESS
String            user_config   ="admin";
String            pwd_config    ="admin"; 
//FUNÇÕES METODOS           
void              initDisplay();
void              everySeconds();
void              wakeUpSong();
void              desativateInterrupt(); 
                  
void setup(void)
{
  pinMode(D1,OUTPUT);
  timerSec.attach(1,everySeconds);
  attachInterrupt(D2,desativateInterrupt,RISING);
  initDisplay();
  Serial.begin(115200);
  Serial.println("**********DESPERTADOR***********");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssidAP,pwdAP);
  WiFi.begin(ssidClient,pwdClient);
  while(WiFi.status() != WL_CONNECTED)
  {
    Serial.println(".");
    delay(500);
  }
  Serial.println("Connecting to NTP Server");
  NTP a(-2);
  setTime(a.getNtpTime());
  hh  = hour();
  mm  = minute();
  ss  = second();
  Serial.println("Atualized Time!");
  
  analogWriteFreq(30000);
  analogWriteRange(256);
  Serial.begin(115200);
  Serial.print("Begin File System");
  if(SPIFFS.begin())
  {
    Serial.print("SPIFFS ok");
  }else
  {
    Serial.print("Error incialize SPIFFS");
  }
  
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) 
  {
    Serial.printf(" %s - %u bytes\n", dir.fileName().c_str(), dir.fileSize());
  }
}

void loop()
{
//  if(timeUp[0] == hour() && timeUp[1] == minute() && desativate){
//    despertar.attach(6,wakeUpSong); 
//  }else{
//    despertar.detach();
//  }
  loop_config();  
}

void wakeUpSong()
{
  file = SPIFFS.open("/alarme.wav","r");
  if(file)
  {
    Serial.println("Success Open File!");
  }else
  {
    Serial.println("Failed open File!");
  }
  int date;
  unsigned long timer;
  date = file.read();
  while(file  && date != -1)
  {
    analogWrite(D1,date);
    timer = micros();
    while((micros() - timer) < 80)
    {
      yield();  
    }
  date = file.read();
  }
  file.close();
  analogWrite(D1,0); 
}
void desativateInterrupt()
{
  desativate = false;
}
void initDisplay()
{
  tft.init();
  tft.setRotation(2);
  tft.fillScreen(TFT_GREY);  
  tft.setTextColor(TFT_WHITE, TFT_GREY);  // Adding a background colour erases previous text automatically
  tft.fillCircle(120, 120, 118, TFT_GREEN);
  tft.fillCircle(120, 120, 110, TFT_BLACK);

  // Draw 12 lines
  for(int i = 0; i<360; i+= 30) 
  {
    sx = cos((i-90)*0.0174532925);
    sy = sin((i-90)*0.0174532925);
    x0 = sx*114+120;
    yy0 = sy*114+120;
    x1 = sx*100+120;
    yy1 = sy*100+120;
    tft.drawLine(x0, yy0, x1, yy1, TFT_GREEN);
  }

  // Draw 60 dots
  for(int i = 0; i<360; i+= 6) 
  {
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
    if (ss==60) 
    {
      ss=0;
      mm++;           
      if(mm>59) 
      {
        mm=0;
        hh++;         
        if (hh>23) 
        {
          hh=0;
        }
      }
    }
    sdeg = ss*6;                  // 0-59 -> 0-354
    mdeg = mm*6+sdeg*0.01666667;  // 0-59 -> 0-360 - includes seconds
    hdeg = hh*30+mdeg*0.0833333;  // 0-11 -> 0-360 - includes minutes and seconds
    hx = cos((hdeg-90)*0.0174532925);    
    hy = sin((hdeg-90)*0.0174532925);
    mx = cos((mdeg-90)*0.0174532925);    
    my = sin((mdeg-90)*0.0174532925);
    sx = cos((sdeg-90)*0.0174532925);    
    sy = sin((sdeg-90)*0.0174532925);

    if (ss==0 || initial) 
    {
      initial = 0;
      tft.drawLine(ohx, ohy, 120, 121, TFT_BLACK);
      ohx = hx*62+121;    
      ohy = hy*62+121;
      tft.drawLine(omx, omy, 120, 121, TFT_BLACK);
      omx = mx*84+120;    
      omy = my*84+121;
    }
    tft.drawLine(osx, osy, 120, 121, TFT_BLACK);
    osx = sx*90+121;    
    osy = sy*90+121;
    tft.drawLine(osx, osy, 120, 121, TFT_RED);
    tft.drawLine(ohx, ohy, 120, 121, TFT_WHITE);
    tft.drawLine(omx, omy, 120, 121, TFT_WHITE);
    tft.drawLine(osx, osy, 120, 121, TFT_RED);
    tft.fillCircle(120, 121, 3, TFT_RED);

    tft.setCursor(40,280);
    String timeWakeUpString = String(timeUp[0]);
    timeWakeUpString += ":";
    timeWakeUpString += String(timeUp[1]);
    tft.println(timeWakeUpString);
    if(timeUp[0] == hour() && timeUp[1] == minute() && desativate){
      despertar.attach(6,wakeUpSong); 
      Serial.println("Alarme Ativate!");
    }else{
      despertar.detach();
    } 
    Serial.printf("hora: %d:%d:%d ",hour(),minute(),second());
    //as

}
//**************FUNÇÃO QUE VERIFICA AUTENTIFICAÇÃO DE LOGIN******************************
//retorna true e false conforme o cookie estiver com o ID correto
bool is_authentified() {
  if (server.hasHeader("Cookie")) 
  {
    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
    Serial.println(cookie);
    if (cookie.indexOf("ESPSESSIONID=1") != -1) 
    {           
      Serial.println("Authentification Successful");
      return true;
    }
  }
  Serial.println("Authentification Failed");
  return false;
}
////*****************ESCREVER HANDLE LOGIN*********************************
void handle_login() {
  String msg;
  if (server.hasHeader("Cookie")) {
    String cookie = server.header("Cookie");
    Serial.println(cookie);
  }
  if (server.hasArg("USERNAME") && server.hasArg("PASSWORD")) 
  {
    if (server.arg("USERNAME") == user_config &&  server.arg("PASSWORD") == pwd_config) 
    {
      server.sendHeader("Location", "/");
      server.sendHeader("Cache-Control", "no-cache");
      server.sendHeader("Set-Cookie", "ESPSESSIONID=1");
      server.send(301);
      return;
    }
    msg = "Wrong username/password! try again.";
  }
  String content = "<html><body><form action='/login' method='POST'>";
  content += "<br>";
  content += "<center>User:<br><input type='text' name='USERNAME' placeholder='user name'><br>";
  content += "<br>Password:<br><input type='password' name='PASSWORD' placeholder='password'><br>";
  content += "<br><br><input type='submit' name='SUBMIT' value='Login'></form>";
  content +=  "<br><h4>"+msg+"</h4>";
  server.send(200, "text/html", content);
}
////*****************FUNÇÃO DA PAGINA INICIAL/CONFIGURAÇÃO*********************************
void handle_setup_page()
{
  String header;
  String hours = String(timeUp[0]);
  String minutes = String(timeUp[1]);
  String hourMissing = String(abs(timeUp[0]-hh));
  hourMissing += ":";
  hourMissing += String(abs(timeUp[1]-mm));

  
  if (!is_authentified())            
  {
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
    return;
  }else 
  {
  String html = "<html><head><title>Configuration </title>";
  html += "<style>body { background-color: #cccccc; ";
  html += "font-family: Arial, Helvetica, Sans-Serif; ";
  html += "Color: #000088; }</style>";  
  html += "</head><body>";
  html += "<h1><center>Configuração de Hora</h1>";                     //1 center foi suficiennte
 
  html += "<p><center>Hora P/ Despertar</p>";                                                      //campo para obter ssid da rede wifi com acesso a internet
  html += "<center><form method='POST' action='/config_save'>";                       //config_salve
  html += "<input type=text name=hourWakeUp placeholder='" + hours + "'/> ";

  html += "<p><center>minuto P/ Despertar</p>";                                                      //campo para obter ssid da rede wifi com acesso a internet
  html += "<center><form method='POST' action='/config_save'>";                       //config_salve
  html += "<input type=text name=minuteWakeUp placeholder='" + minutes + "'/> ";

  html += "<p><center>Time to Wake Up</p>";                                                      //campo para obter ssid da rede wifi com acesso a internet
  html += "<center> " + hourMissing + "</p>";                         
  
  html += "</p>";
  html += "<input type=submit name=botao value=Enviar /></p>";  
  html += "</form>"; 
  html += "</body></html>";
  server.send(200, "text/html", html);
  }
}
//*****************FUNÇÃO DA PAGINA DE SALVAMENTO DA CONFIGURAÇÃO************************  
  void handle_configuration_save()
  {
  String hours;
  String html =  "<html><head><title>Saved Settings</title>";
  html += "<style>body { background-color: #cccccc; ";
  html += "font-family: Arial, Helvetica, Sans-Serif; ";
  html += "Color: #000088; }</style>";
  html += "</head><body>";
  html += "<h1><center>Saved Settings</h1>";
  html += "</body></html>";
  server.send(200, "text/html", html);
  if(server.arg("hourWakeUp") != ""){
     timeUp[0]     = atoi(server.arg("hourWakeUp").c_str());
  }
  if(server.arg("minuteWakeUp") != ""){
     timeUp[1]     = atoi(server.arg("minuteWakeUp").c_str());
  }
                    
}

//********************FUNÇÃO QUE EXECUTA A PARTE DE CONFIGURAÇÃO***********************
void loop_config()
{
    delay(2000);                                                        //2 segundos para filtrar ruido do botao
    Serial.println(WiFi.localIP());
    server.on("/", handle_setup_page);
    server.on("/login",handle_login);
    server.on("/config_save",handle_configuration_save);
    server.on("/inline", []() {server.send(200, "text/plain", "this works without need of authentification"); });
    const char * headerkeys[] = {"User-Agent", "Cookie"} ;
    size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);  
    server.collectHeaders(headerkeys, headerkeyssize);
    server.begin();
    server.sendHeader("Set-Cookie", "ESPSESSIONID=1");        //precisei forçar esta flag para rodar o login durente teste
    server.handleClient();
    while(true)
    {
      server.handleClient();
    }
  }
