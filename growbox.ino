#include "DHT.h" //library for humidity and temperature sensor
#include <SPI.h>
//#include <SD.h>  // include the SD library:
#include <Wire.h>
#include "ssd1306.h" //library for lcd display
#include "floatToString.h" //library for convertation float to string
#include "RTClib.h" //library for real time clock
RTC_DS1307 rtc;

// set up variables using the SD utility library functions:
//Sd2Card card;
//SdVolume volume;
//SdFile root;

#define VCCSTATE SSD1306_SWITCHCAPVCC
#define WIDTH     128
#define HEIGHT     64
#define PAGES       8

/* pins for lcd display */
#define OLED_DC     8    //brown
#define OLED_RST    9    //black
#define OLED_CS    10    //red
#define SPI_MOSI   51    /* connect to the DIN pin of OLED yellow*/
#define SPI_SCK    52    /* connect to the CLK pin of OLED orange */

#define DHTPIN     2  // pin for humidity and temperature sensor
#define buttonPin1 3  //button1 pin 3
#define lightPin   4  //220v 3x LED light pin
#define o_Motor    5  // pin for motor (290ml for 30 seconds = 4 volts)
#define buttonPin2 A2 //button2 pin A2 
#define buttonPin3 A3 //button3 pin A3
#define coolers    A0 //Collers relay
#define humid      A1 //Humidifier relay


#define DHTTYPE DHT21   // DHT 21 (AM2301) type of humidity and temperature sensor

uint8_t oled_buf[WIDTH * HEIGHT / 8]; //LCD display

DHT dht(DHTPIN, DHTTYPE, 6); //Sensor library

byte buttonState1 = 0;  //set button 1 not pressed
byte buttonState2 = 0;  //set button 2 not pressed
byte buttonState3 = 0;  //set button 3 not pressed

int lcd_timer = -1;

byte menu = 0; 
char* name_menu[] = {"TIME","TEST","GROW"};

float h; //variable for humidity
float t; //variable for temperature

unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;


void setup() 
{
  Serial.begin(9600);
  //Serial3.begin(115200);
  rtc.begin();
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

  dht.begin(); //start sensor
  SSD1306_begin(); //start display
  pinMode(o_Motor, OUTPUT); // for mini pump
  pinMode(lightPin, OUTPUT); //220v Light
  pinMode(coolers, OUTPUT); //Coolers relay
  pinMode(humid, OUTPUT); //Humidifier relay

  pinMode(buttonPin1, INPUT); //for button1
  pinMode(buttonPin2, INPUT); //for button2
  pinMode(buttonPin3, INPUT); //for button3




}
void loop() 
{

  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis1 >= 2000) //read temperature and humidity every 2 seconds
  { 
    previousMillis1 = currentMillis;
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    h = dht.readHumidity();
    t = dht.readTemperature();
    Serial.print(t);
    Serial.print(" TEMP C \n");
    Serial.print(h);
    Serial.print(" HUM %  \n");  
  }

  buttonState1 = digitalRead(buttonPin1); //reading state of button1
  buttonState2 = digitalRead(buttonPin2); //reading state of button2
  buttonState3 = digitalRead(buttonPin3); //reading state of button3

  DateTime now = rtc.now(); //read datetime

  grow(); //start growning

  // check if returns are valid, if they are NaN (not a number) then something went wrong!
  if (isnan(t) || isnan(h))
  {
    Serial.println("Failed to read from DHT sensor");
  }
  if (buttonState1 == HIGH || buttonState2 == HIGH || buttonState3 == HIGH) 
  {  
    if ((buttonState1 == HIGH && buttonState2 != HIGH && buttonState3 != HIGH)||
        (buttonState2 == HIGH && buttonState1 != HIGH && buttonState3 != HIGH)||
        (buttonState3 == HIGH && buttonState1 != HIGH && buttonState2 != HIGH)) //check if its not noice
    {   
      Serial.println("Button pressed!");
      Serial.println(now.hour());
      Serial.println(now.minute());
      Serial.println(now.second());
      if (now.second() < 30) //set timer to 30 seconds
      { 
        lcd_timer = now.second() + 30;
      }
      else
      {
        lcd_timer = now.second() - 30;
      }
    }
  }

 

  if (now.second() != lcd_timer && lcd_timer != -1)//show display for 30 sec.
  { 
    SSD1306_clear(oled_buf); //clear display
    show_menu(currentMillis);
    show_sensor (h, t); //Show info from sensor on display
    show_time(); //data from real time clock
    SSD1306_display(oled_buf); //show info on display
  } 
  else //disable screen after 30 seconds
  { 
    SSD1306_clear(oled_buf);//clear display
    SSD1306_display(oled_buf);//show info on display
    lcd_timer = -1;
  }
}
void show_menu(unsigned long currentMillis)
{
  previousMillis2 = currentMillis;
  
  if(buttonState3 == HIGH && menu < 2) //length of menu is 3 
  {
    menu += 1;
    delay(200);
  }
  if(buttonState2 == HIGH && menu > 0) 
  {
    menu -= 1;
    delay(200);
  }
  if(buttonState1 == HIGH && buttonState2 != HIGH && menu == 1) 
  {
    test_all_devices(currentMillis); //start all devices for testing
  }

  
  SSD1306_string(50, 40, name_menu[menu], 12, 0, oled_buf);    
}

void show_sensor (float h, float t) 
{
  char str[6]; //data from sensor about temp or humidity

  /* display temperature */
  floatToString(str, t, 2);
  //SSD1306_string(0, 0,"TEMP:", 12, 1, oled_buf);
  SSD1306_bitmap(0, 2, Temperature1010, 10, 10, oled_buf);
  SSD1306_string(13, 0, str, 12, 1, oled_buf);
  SSD1306_bitmap(45, 1, Celsius1010, 10, 10, oled_buf);

  /* display humidity */
  floatToString(str, h, 2);
  //SSD1306_string(70, 0, "HUM:", 12, 1, oled_buf);
  SSD1306_bitmap(73, 1, Drop1010, 10, 10, oled_buf);
  SSD1306_string(86, 0, str, 12, 1, oled_buf);
  SSD1306_bitmap(116, 2, Percent1010, 10, 10, oled_buf);
}

void show_time() 
{
  char str[2];

  DateTime now = rtc.now();
  /* display time */

  sprintf(str, "%d", now.hour());

  if (now.hour() < 10) 
  {
    SSD1306_char1616(0, 16, '0', oled_buf); //SSD1306_char1616
    SSD1306_char1616(16, 16, str[0], oled_buf);
  }
  else 
  {
    SSD1306_char1616(0, 16, str[0], oled_buf);
    SSD1306_char1616(16, 16, str[1], oled_buf);
  }

  SSD1306_char1616(32, 16, ':', oled_buf); //divider of hours ":"

  sprintf(str, "%d", now.minute());

  if (now.minute() < 10) 
  {
    SSD1306_char1616(48, 16, '0', oled_buf);
    SSD1306_char1616(64, 16, str[0], oled_buf);
  }
  else 
  {
    SSD1306_char1616(48, 16, str[0], oled_buf);
    SSD1306_char1616(64, 16, str[1], oled_buf);
  }

  SSD1306_char1616(80, 16, ':', oled_buf); //divider of minutes ":"

  sprintf(str, "%d", now.second());

  if (now.second() < 10) 
  {
    SSD1306_char1616(96, 16, '0', oled_buf);
    SSD1306_char1616(112, 16, str[0], oled_buf);
  }
  else 
  {
    SSD1306_char1616(96, 16, str[0], oled_buf);
    SSD1306_char1616(112, 16, str[1], oled_buf);
  }
}

void grow() 
{
  DateTime now = rtc.now();
  unsigned long currentMillis = millis();
  int day_today = now.day(); //current day

  if (now.hour() >= 6 && now.hour() <= 21 ) //set lights to 16 hours from 6am. to 22pm.(tested)
  {
    digitalWrite(lightPin, HIGH);
  }
  else
  {
    digitalWrite(lightPin, LOW);
  }

  if (now.hour() >= 6 && now.hour() <= 21 ) //set coolers to 16 hours from 6am. to 22pm.
  {
    digitalWrite(coolers, HIGH);
  }
  else if ((now.hour() >= 22 && now.minute() >= 15) && now.hour() <= 5) //disable coolers 15 min after
  {
    digitalWrite(coolers, LOW);
  }

  if ((day_today % 2 == 0 && now.hour() == 23 && now.minute() == 59 && now.second() <= 30) ||
      (day_today % 2 != 0 && now.hour() == 4 && now.minute() == 59 && now.second() <= 30)) //enable water pump for 30 sec at 5 am. and 12pm.
  {
    digitalWrite(o_Motor, HIGH);  // enable water pump
  }
  else
  {
    digitalWrite(o_Motor, LOW);  // disable water pump
  }

  if (t > 60) //in case of high temperature disable light and coolers
  {
    digitalWrite(lightPin, LOW);
    digitalWrite(coolers, LOW);
  }

  if (h < 35 && h != 0) //enable humidifier if humidity not enought
  {
    digitalWrite(humid, HIGH);
  }
  else if (h >= 55 || h == 0)
  {
    digitalWrite(humid, LOW);
  }

}

void test_all_devices(unsigned long currentMillis) 
{
  if (currentMillis - previousMillis2<=5000) 
  {
    digitalWrite(o_Motor, HIGH);  // enable engine
    digitalWrite(humid, HIGH); //start humidifier
    digitalWrite(lightPin, HIGH); //disable light
    digitalWrite(coolers, HIGH);
  }
  else 
  {  
    digitalWrite(o_Motor, LOW); // disable engine
    digitalWrite(humid, LOW); //start
    digitalWrite(lightPin, LOW); //disable light
    digitalWrite(coolers, LOW);
  }
}
