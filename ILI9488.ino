/** ESP-32 and API dependencies*/
#include <WiFi.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <TimeLib.h>
#include <time.h>

/**Display dependencies*/
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "ILI9486_SPI.h"

/** Display pins*/
#define TFT_DC 4
#define TFT_CS 15
#define TFT_RST 2
#define TFT_MISO 19         
#define TFT_MOSI 23           
#define TFT_CLK 18 

//ILI9486_SPI tft(TFT_CS, TFT_DC, TFT_RST);


ILI9486_SPI tft(/*CS=5*/ 15, /*DC=*/ 4, /*RST=*/ 2);

#if !defined(ESP8266)
#define yield()
#endif

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

const char * ssid = "NETIASPOT-2.4GHz-71EB2C";
const char * wifipw = "2M3PvcGXtsny";
String country= "ee"; // "ee" - Eesti, "lv" - Läti, lt - "Leedu", "fi" - Soome

void setTimezone(const char * timezone) {
  Serial.print("Setting Timezone to ");
  Serial.println(timezone);
  setenv("TZ", timezone, 1);
  tzset();
}

void initTime(const char * timezone) {
  struct tm timeinfo;

  Serial.println("Getting time from NTP server");
  configTime(0, 0, "1.pool.ntp.org");    // First connect to NTP server, use 0 TZ offset
  delay(2000); //MUUDETUD: VÕTSIN ÜHE NULLI VÄHEMAKS
  if (!getLocalTime(&timeinfo),5) { 
    Serial.println("  Failed to obtain time");
    setTimezone(timezone);
    return;
  }
  Serial.println("OK, Got the time from NTP");
  setTimezone(timezone); // then set your timezone
}

long printLocalTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo,5)) { 
    Serial.println("Failed to obtain time");
    return 0;
  }
  Serial.println(&timeinfo); //, "%A, %B %d %Y %H:%M:%S zone %Z %z "
  time(&now);
  Serial.println(now); //25200 on adjustment USA idaranniku ajast Eesti aega.
  setTime(now);
  delay(50);
  return now;
}

String makeTodayTimestamp(){ 
  return String(year())+"-"+ 
        (month() < 10 ? "0"+String(month()) : String(month()))+"-" + 
        (day() < 10 ? "0"+String(day()) : String(day()))+"T"+
        (hour()-1 < 10 ? "0"+String(hour()-1) : String(hour()-1))+"%3A"+
        (minute() < 10 ? "0"+String(minute()) : String(minute()))+"%3A"+
        (second() < 10 ? "0"+String(second()) : String(second())) + ".999Z";
}

String makeTomorrowTimestamp(){
  return String(year())+"-"+ 
        (month() < 10 ? "0"+String(month()) : String(month()))+"-" + 
        (day() < 9 ? "0"+String(day()+1) : String(day()+1))+"T"+
        (hour() < 10 ? "0"+String(hour()) : String(hour()))+"%3A"+
        (minute() < 10 ? "0"+String(minute()) : String(minute()))+"%3A"+
        (second() < 10 ? "0"+String(second()) : String(second())) + ".999Z";
}

String unixToUTC2Hour(long timestamp){
  int hour= ((timestamp+7200)/3600)%24;
  if(hour>=22){
    return String(hour);
  }
  else if (hour==0) {
    return "23"; 
  }
  else if (hour==1){
    return "00";
  }
  else{
    return (hour < 10 ? "0"+String(hour) : String(hour));
  }
}

String nextHour(String curr_hour){
  int hour=curr_hour.toInt()+1;
  if(hour==24){
    return "00";
  }
  else{
    return (hour < 10 ? "0"+String(hour) : String(hour));
  }
}

void displayPrices(String hours[], float prices[], String max_time, float max_price, String min_time, float min_price, float avg){
  Serial.println("Starting display!");
  tft.setSpiKludge(false);
  tft.init();

  tft.setRotation(1);
  tft.fillScreen(BLACK);
  tft.drawLine(200,0,200,320, WHITE);
  tft.drawLine(200,120,480,120, WHITE);
  tft.drawLine(340,0,340,120, WHITE);

  // Display the minimum price
  displayMinPrice(min_price, min_time);

  // Display the maximum price
  displayMaxPrice(max_price, max_time);

  //Display the previous, current and next three hours
  displayHourly(hours, prices);

  //Display the diagram
  displayDiagram(hours, prices, max_price, min_price, avg);
  
  delay(100000);
  return;
}

void displayMinPrice(float min_price, String min_time){
  tft.setCursor(240,25);
  tft.setTextColor(GREEN); tft.setTextSize(2);
  tft.println(min_time+":00");

  tft.setCursor(210+calc_y_offset(min_price),50);
  tft.setTextColor(GREEN); tft.setTextSize(3);
  tft.println(min_price);
}

void displayMaxPrice(float max_price, String max_time){
  tft.setCursor(390,25);
  tft.setTextColor(RED); tft.setTextSize(2);
  tft.println(max_time+":00");

  tft.setCursor(360+calc_y_offset(max_price),50);
  tft.setTextColor(RED); tft.setTextSize(3);
  tft.println(max_price);
}

void displayHourly(String hours[], float prices[]){

  int y_correct=0;
  for(int i=0;i<4;i++){

  tft.setCursor(8, 40+y_correct);
  
  tft.setTextColor(i==0 ? BLUE : WHITE); tft.setTextSize(2);
  tft.print(hours[i]+":00");

  tft.setCursor(85,35+y_correct);
  tft.setTextColor(i==0 ? BLUE : WHITE); tft.setTextSize(3);
  tft.print(prices[i]);

  y_correct+=75;
  }

}

void displayDiagram(String hours[], float prices[], float max_price, float min_price, float avg){
  float height=0;
  for (int i=0; i<12; i++){
    float price=prices[i];
    if (price/2>168){
      height=168;
    }
    else{
      height=price/2;
    }
    
    tft.drawRect(214+21*i,280-height , 21, height, WHITE);
    tft.fillRect(214+21*i,280-height , 21, height, height>avg/2 ? RED : height<avg/2 ? GREEN : i==0 ? BLUE : WHITE);

  }

  tft.setRotation(0);
  for (int i=0;i<12;i++){
    tft.setCursor(5,220+21*(i));
    tft.setTextColor(prices[i] == max_price ? RED : prices[i] == min_price ? GREEN : i==0 ? BLUE : WHITE); tft.setTextSize(1);
    tft.print(hours[i]+":00");

    tft.setCursor(prices[i]/2+50, 220+21*(i));
    tft.setTextColor(prices[i] == max_price ? RED : prices[i] == min_price ? GREEN : i==0 ? BLUE : WHITE); tft.setTextSize(1);
    tft.print(prices[i]);
  }
  tft.setRotation(1);

  int start=210;
  for(int s=0; s<23;s++){
    tft.drawLine(start,280-avg/2,start+6,280-avg/2, YELLOW);

    start+=12;
  }
}

int calc_y_offset(float n){
  int len=String(n).length();
  Serial.println(String("Arv "+ String(n)+ "on "+len+" numbri pikkune."));
  if(len==6){
    return 0;
  }
  else if(len==5){
    return 10;
  }
  else if(len==4){
    return 20;
  }
  else if(len==3){
    return 30;
  }
  else if(len==2){
    return 40;
  }
  else{
    return 45;
  }

}

void ApiWork(long timestamp){
  HTTPClient client;
  Serial.println(makeTodayTimestamp());
  Serial.println(makeTomorrowTimestamp());
  client.begin("https://dashboard.elering.ee/api/nps/price?start="+makeTodayTimestamp()+"&end="+makeTomorrowTimestamp()); //https://dashboard.elering.ee/assets/api-doc.html#/nps-controller/getPriceUsingGET
  int httpCode = client.GET();
  Serial.println("\nStatuscode: " + String(httpCode));
  
  if (httpCode > 0){
    String input = client.getString();
    Serial.println("\nStatuscode: " + String(httpCode));
    Serial.println(input);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, input);

    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
    return;
    }

    bool success = doc["success"]; // true

    JsonObject data = doc["data"];    

    /****** Loeb JSONi sisse *******/
  
    int len=0;
    for (JsonObject data_ee_item : data[country].as<JsonArray>()) { //https://arduinojson.org/v7/assistant/#/step3
      len++;
    }

    String hours[len];
    float prices[len];
    
    String hour= unixToUTC2Hour(timestamp);
    int i=0;
    for (JsonObject data_ee_item : data[country].as<JsonArray>()) { //https://arduinojson.org/v7/assistant/#/step3
      String data_hour = hour;
      float data_price = data_ee_item["price"]; // 3.88, 4.54, 4.16, 4.54, 4.56, 3.97, 5.63, 29.49, ...

      prices[i]=data_price;
      hours[i]=data_hour;

      i++;
      hour=nextHour(hour);
    }
    /************/

    Serial.println("Praegune ("+ hours[0] +") hind on: " + String(prices[0]));
    Serial.println("Järgmise tunni ("+hours[1] + ")hind on: " + String(prices[1]));
    Serial.println("Ülejärgmise ("+ hours[2] +") tunni hind on: " + String(prices[2]));
    

    float highest_price=0;
    int highest_id;
    float lowest_price= prices[0];
    int lowest_id; 
    float total=0;

    for (int a=0; a<(sizeof(prices)/4); a++){
      total+=prices[a];
      if (prices[a]>=highest_price){
        highest_price=prices[a];
        highest_id=a;
      }
      if (prices[a]<=lowest_price){
        lowest_price=prices[a];
        lowest_id=a;
      }
    }

    float avg=total/(sizeof(prices)/4);

    Serial.println("Keskmine hind on: " +String(avg));

    Serial.println("Kõrgeim hind on " + String(highest_price) + " ajahetkel " + hours[highest_id]);
    Serial.println("Madalaim hind on " + String(lowest_price) + " ajahetkel " + hours[lowest_id]);
    
    Serial.println();
    for(int b=0;b<sizeof(prices)/4;b++){
      Serial.println(hours[b] + " - " + prices[b]);
    }



    /** IMPORTANT INFORMATION **/

    float current_hour_price=prices[0];
    float next_hour_price= prices[1];
    float hour_after_next_hour_price= prices[2];
    //highest_price, lowest_price;
    String highest_price_time=hours[highest_id];
    String lowest_price_time=hours[lowest_id];

    displayPrices(hours, prices, highest_price_time, highest_price, lowest_price_time, lowest_price, avg);
  }
  else {
    Serial.println("Error on HTTP request");
  }
  
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  WiFi.begin(ssid, wifipw);
  Serial.println("Connecting Wifi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWifi OK");

  initTime("CET-2CEST,M3.2.0,M11.1.0"); // https://sites.google.com/a/usapiens.com/opnode/time-zones
}

void loop() {
  long timestamp=printLocalTime();
  ApiWork(timestamp);
  delay(1000000);
}


