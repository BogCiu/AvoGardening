/*    Desired equipment         | Status                  | Type

   Soil sensor                  | 4                       | Input
   Air sensor + humidity        | 4                       | Input
   Light sensor                 | 4                       | Input
   RTC                          | 4                       | Input
   Watering Pump                | 4                       | Output
   Evacuation Pump              | 4                       | Output
   Water Sensor                 | 4                       | Input
   Inherent WiFi functionality  | 4                       | N/A

*/

/*   Equipment code implementation state

   0. Not started
   1. Reading values
   2. Confirming and calibrating values - where applicable
   3. Logic implementation
   4. Confirmed finished code writing

*/

#include <ThreeWire.h>// Library required for RTC module
#include <RtcDS1302.h>// Library required for RTC module
#include <WiFiNINA.h> // Library required for WiFi functionality
#include "DHT.h" // Library required for DHT air humidity and temperature sensor

#define DHTPIN 3
#define DHTTYPE DHT11
#define SoilHumidityA A5
#define SoilHumidityD 2
#define LightSensor A4 //voltage divisor with 10kOhm resistor
#define RTCCLK 8
#define RTCDAT 5
#define RTCRST 4
#define WaterPower 7
#define WaterLevel A3
#define PumpWater 9
#define PumpEvac 10
#define Fan 11

RtcDateTime end_time_water_pump;
RtcDateTime end_time_evac_pump;
RtcDateTime end_time_timed_Wpump;
RtcDateTime end_time_timed_Epump;
RtcDateTime DryCycEnd;
RtcDateTime WetCycEnd;
RtcDateTime sens_end_water_time;
RtcDateTime fan_end_time;
RtcDateTime end_time_auto_Epump;
bool write_to_serial = false;
bool sent_for_next_minute = false;
bool weekly_or_sensor = false;
bool pump_water_set_duration = false;
bool pump_evac_set_duration = false;
bool pump_water_scheduled_duration = false;
bool pump_evac_scheduled_duration = false;
bool sensor_watering_state=false;
bool auto_Epump_state = false;
bool auto_timed_watering[7][5];
int auto_timed_hour[7][5];
int auto_timed_minute[7][5];
int auto_timed_duration[7][5];
int waterlevel_value = 0; //used in void loop() reading
int waterpercent = 0; //used in void loop() reading
int maxwatervalue = 0; //used in void loop() reading ; Find out the maximum water level that will exist in the collection tray for further use in logic
int evac_water_level = 70;
int calibrated_max_water_level = 550;
int total_watering_time = 500; //seconds
int nr_watering_intervals = 5; //default 100 ml 5 times / day.
int interval_start = 8; // time after which watering can happen (does not work with 0)
int interval_stop = 22; // time before which watering can happen (does not work with 23)
int minute_to_water[20] = {0};
int hour_to_water[20] = {0};
int hour_carryover = 0;
char day_of_the_week[10] = "Monday";
char datestring[20];
char monday_auxiliary_href[5][31]={"<a href=\"?Del00\">Starting at ","<a href=\"?Del01\">Starting at ","<a href=\"?Del02\">Starting at ","<a href=\"?Del03\">Starting at ","<a href=\"?Del04\">Starting at "};
char tuesday_auxiliary_href[5][31]={"<a href=\"?Del10\">Starting at ","<a href=\"?Del11\">Starting at ","<a href=\"?Del12\">Starting at ","<a href=\"?Del13\">Starting at ","<a href=\"?Del14\">Starting at "};
char wednesday_auxiliary_href[5][31]={"<a href=\"?Del20\">Starting at ","<a href=\"?Del21\">Starting at ","<a href=\"?Del22\">Starting at ","<a href=\"?Del23\">Starting at ","<a href=\"?Del24\">Starting at "};
char thursday_auxiliary_href[5][31]={"<a href=\"?Del30\">Starting at ","<a href=\"?Del31\">Starting at ","<a href=\"?Del32\">Starting at ","<a href=\"?Del33\">Starting at ","<a href=\"?Del34\">Starting at "};
char friday_auxiliary_href[5][31]={"<a href=\"?Del40\">Starting at ","<a href=\"?Del41\">Starting at ","<a href=\"?Del42\">Starting at ","<a href=\"?Del43\">Starting at ","<a href=\"?Del44\">Starting at "};
char saturday_auxiliary_href[5][31]={"<a href=\"?Del50\">Starting at ","<a href=\"?Del51\">Starting at ","<a href=\"?Del52\">Starting at ","<a href=\"?Del53\">Starting at ","<a href=\"?Del54\">Starting at "};
char sunday_auxiliary_href[5][31]={"<a href=\"?Del60\">Starting at ","<a href=\"?Del61\">Starting at ","<a href=\"?Del62\">Starting at ","<a href=\"?Del63\">Starting at ","<a href=\"?Del64\">Starting at "};\
float min_plant_temp = 25;
float max_plant_temp = 38;
int fan_duration = 20;
bool fan_state=false;
bool enable_fan = true;
bool sens_ana_dig=false;
bool dry_cyc_active=false;
bool wet_cyc_active=false;
int sens_hum_trigger=30;
int sens_dry_cyc_d=1;
int sens_dry_cyc_h=12;
int sens_dry_cyc_m=0;
int sens_dry_cyc_s=0;
int sens_base_dur_s=350;
int sens_watering_time=350;

char compiled_time[20];
int pump_duration = 0;

char ssid[] = "Voicu 2.4GHz"; //Data for wireless network to connect to
char pass[] = "cocovoicu";    //Data for wireless network to connect to
int status = WL_IDLE_STATUS;
WiFiServer server(80);

String readString;

ThreeWire myWire(RTCDAT, RTCCLK, RTCRST);
RtcDS1302<ThreeWire> Rtc(myWire);
DHT dht(DHTPIN, DHTTYPE);
void setup() {

  for(int i=0;i<7;i++){
    for(int j=0;j<5;j++){
      auto_timed_watering[i][j] = false;
    }
  }
  for(int i=0;i<7;i++){
    for(int j=0;j<5;j++){
      auto_timed_hour[i][j] = 24;
    }
  }
  for(int i=0;i<7;i++){
    for(int j=0;j<5;j++){
      auto_timed_minute[i][j] = 60;
    }
  }
  for(int i=0;i<7;i++){
    for(int j=0;j<5;j++){
      auto_timed_duration[i][j] = 0;
    }
  }
  
  Serial.begin(9600);

  Serial.print("compiled: ");
  Serial.print(__DATE__);
  Serial.print("-");
  Serial.println(__TIME__);

  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  String time_compiled = printDateTime(compiled);
  Serial.println(time_compiled);

  if (!Rtc.IsDateTimeValid())
  {
    // Common Causes:
    //    1) first time you ran and the device wasn't running yet
    //    2) the battery on the device is low or even missing

    Serial.println("RTC lost confidence in the DateTime!");
    Rtc.SetDateTime(compiled);
  }

  if (Rtc.GetIsWriteProtected())
  {
    Serial.println("RTC was write protected, enabling writing now");
    Rtc.SetIsWriteProtected(false);
  }

  if (!Rtc.GetIsRunning())
  {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled)
  {
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
    Rtc.SetDateTime(compiled);
  }
  else if (now > compiled)
  {
    Serial.println("RTC is newer than compile time. (this is expected)");
  }
  else if (now == compiled)
  {
    Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }

  pinMode(PumpWater, OUTPUT);
  pinMode(PumpEvac, OUTPUT);
  pinMode(Fan, OUTPUT);
  digitalWrite(PumpWater, HIGH);
  digitalWrite(PumpEvac, HIGH);
  digitalWrite(Fan, HIGH);
  pinMode(SoilHumidityA, INPUT);
  pinMode(SoilHumidityD, INPUT);
  pinMode(WaterLevel, INPUT);
  pinMode(WaterPower, OUTPUT);
  digitalWrite(WaterPower, LOW);

  //Since the Water level Sensor will be regularly powered off to extend the lifetime of the sensor, and it's variables are initialized at 0, we'll read the water level variables once at setup so they start at correct readings, and not at 0
  digitalWrite(WaterPower, HIGH);
  delay(10);
  waterlevel_value = analogRead(WaterLevel);
  digitalWrite(WaterPower, LOW);
  maxwatervalue = calibrated_max_water_level;
  waterpercent = map(waterlevel_value, 200, calibrated_max_water_level, 0, 100); // If water sensor is new /not corroded and completely dry, value reads 0, but the instant it gets wet, the value starts reading around 200~240, therfore percentage calculation starts at 0
  waterpercent = max(waterpercent,0); // if the map function gives it a negative function because the reading is lower than 200, percentage is 0% (only possible if sensor is compeltely dry and with no corrosion on it

  dht.begin();

  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(10000);
  }
  server.begin();

  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

void loop() {

  delay(50);
  // SOIL HUMIDITY READING AND CALIBRATION
  // Analog pin is used to calibrate digital pin, then user can remove analog for more analog inputs/outputs

  int soil_humidity = analogRead(SoilHumidityA); // Soil Humidity reading
  bool soil_humidity_state = digitalRead(SoilHumidityD); // Soil Humidity DO reading for calibration
  soil_humidity = map(soil_humidity, 0, 1023, 100, 0);

  // DHT air temperature and humidity sensor

  float air_humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  if (isnan(air_humidity) || isnan(temperature)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  float heat_index_celsius = dht.computeHeatIndex(temperature, air_humidity, false);

  // Light Sensor reading

  int light = analogRead(LightSensor);

  // RTC write time

  RtcDateTime now = Rtc.GetDateTime();

  //Check current temperature compared to optimal values for plant and turn on fan if too hot.
  if(enable_fan==true)
  {
    if((temperature>max_plant_temp) && (fan_state==false))
    {
      fan_state=true;
      digitalWrite(Fan,LOW);
      fan_end_time = calculateEndTime(now,fan_duration);
    }
    if((temperature<min_plant_temp) || (now>fan_end_time))
    {
      if(temperature<=max_plant_temp)
      {
        fan_state=false;
        digitalWrite(Fan,HIGH);
      }
    }
  }
  if(enable_fan==false)
  {
    fan_state=false;
    digitalWrite(Fan,HIGH);
  }
  //Check if timer for pumps set ON for a certain ammount of time is expired, if yes, stop the pumps
  if((pump_water_set_duration == true) && (end_time_water_pump<now))
  {
    pump_water_set_duration = false;
    digitalWrite(PumpWater, HIGH);    
  }
    if((pump_evac_set_duration == true) && (end_time_evac_pump<now))
  {
    pump_evac_set_duration = false;
    digitalWrite(PumpEvac, HIGH);    
  }

  if (int(now.Second()) >= 58)
  {
    sent_for_next_minute = false;
  }
  if ((int(now.Second()) <= 2) && (sent_for_next_minute == false))
  {
    sent_for_next_minute = true;
    write_to_serial = true;
  }
  if(weekly_or_sensor==false)
  {
  AutoWaterTime(now);
  if((pump_water_scheduled_duration == true) && (end_time_timed_Wpump <= now)){
    pump_water_scheduled_duration = false;
    digitalWrite(PumpWater,HIGH);
  }
  if((pump_evac_scheduled_duration == true) && (end_time_timed_Epump <= now)){
    pump_evac_scheduled_duration = false;
    digitalWrite(PumpEvac,HIGH);
  }
  }
  // Water Sensor level
  // Water level reading is only done twice per minute because leaving the sensor turned ON while it is potentially submerged in water will reduce it's lifetime faster

  if (int(now.Second()) ==0 || (int(now.Second()) == 30))
  {
    digitalWrite(WaterPower, HIGH);
    delay(10);
    waterlevel_value = analogRead(WaterLevel);
    maxwatervalue = max(waterlevel_value, maxwatervalue);
    waterpercent = map(waterlevel_value, 200, calibrated_max_water_level, 0, 100);
    waterpercent = max(waterpercent,0);
  }
  else
  {
    digitalWrite(WaterPower, LOW);
  }
  //Automatically turn on Evac Pump if water level is too high
  if((waterpercent>evac_water_level) && (auto_Epump_state == false))
  {
    digitalWrite(PumpEvac,LOW);
    end_time_auto_Epump = calculateEndTime(now,30);
    auto_Epump_state=true;
  }
  if((auto_Epump_state == true) && (now > end_time_auto_Epump))
  {
    if(waterpercent<evac_water_level)
    {
      digitalWrite(PumpEvac,HIGH);
      auto_Epump_state=false;
    }
    else
    {
      end_time_auto_Epump = calculateEndTime(now,30);
    }
  }
  if(weekly_or_sensor==true)
  {
    SensorDryCycle(now,soil_humidity,soil_humidity_state,sens_ana_dig,sens_hum_trigger, sens_dry_cyc_d, sens_dry_cyc_h, sens_dry_cyc_m, sens_dry_cyc_s, sens_base_dur_s);
  }

  //ALL SERIAL MONITOR PRINTING HERE
  if (write_to_serial == true)
  {
    write_to_serial = false;
    Serial.print("At Date ");
    String time_now_1 = printDateTime(now);
    switch(now.DayOfWeek()){
      case 1:strlcpy(day_of_the_week,"Monday",10);
      break;
      case 2:strlcpy(day_of_the_week,"Tuesday",10);
      break;
      case 3:strlcpy(day_of_the_week,"Wednesday",10);
      break;
      case 4:strlcpy(day_of_the_week,"Thursday",10);
      break;
      case 5:strlcpy(day_of_the_week,"Friday",10);
      break;
      case 6:strlcpy(day_of_the_week,"Saturday",10);
      break;
      case 0:strlcpy(day_of_the_week,"Sunday",10);
      break;
      default:strlcpy(day_of_the_week,"Error",10);
      break;
    }
    Serial.print(day_of_the_week);
    Serial.print((now).DayOfWeek());
    Serial.print(" - ");
    Serial.println(time_now_1);
    if (!now.IsValid())
    {
      // Common Causes:
      //    1) the battery on the device is low or even missing and the power line was disconnected
      Serial.println("RTC lost confidence in the DateTime!");
    }

    Serial.print("The soil humidity is:");
    Serial.print(soil_humidity);
    Serial.print(" and the digital output is:");
    Serial.print(soil_humidity_state);
    Serial.println();

    Serial.print("Air humidity is:");
    Serial.print(air_humidity);
    Serial.print(",temperature is:");
    Serial.print(temperature);
    Serial.print(" (C),and the heat index is:");
    Serial.print(heat_index_celsius);
    Serial.println(" (C).");
    Serial.println();

    Serial.print("Light reading value is:");
    Serial.print(light);
    if (light < 10) {
      Serial.println("-Dark");
    } else if (light < 200 ) {
      Serial.println("-Dim");
    } else if (light < 500 ) {
      Serial.println("-Light");
    } else if (light < 800 ) {
      Serial.println("-Bright");
    } else {
      Serial.println("-Very Bright");
    }

    Serial.print("Water level: ");
    Serial.print(waterlevel_value);
    Serial.print(", out of a maximum value of: ");
    Serial.print(maxwatervalue);
    Serial.print(", percentage level: ");
    Serial.print(waterpercent);
    Serial.println(". The calibrated maximum value is 430");
    
    Serial.println(" ");

    Serial.println();
    Serial.println();
  }

  //WiFi data transmit
  String time_now_2 = printDateTime(now);
  WiFiClient client = server.available();
  if (client)
  {
    Serial.println("New request");
    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        if (readString.length() < 100)
        {
          readString += c;
          Serial.write(c);
          if (c == '\n') {
            
            client.print("<html>");
            client.print("<head>");
            client.print("<title>Avocado Garduino</title>");
            client.print("<style>");
            client.print("body{background-image: url('https://images.unsplash.com/photo-1524055988636-436cfa46e59e?ixid=MnwxMjA3fDB8MHxzZWFyY2h8MXx8cGxhbnRzJTIwd2hpdGUlMjBiYWNrZ3JvdW5kfGVufDB8fDB8fA%3D%3D&ixlib=rb-1.2.1&w=1000&q=80');");
            client.print("background-repeat: no-repeat;");
            client.print("background-attachment: fixed;");
            client.print("background-size: 100% 100%;}");
            
            client.print(".switch {position: relative; display: inline-block; width: 40px; height: 20px;}");
            client.print(".switch input {opacity: 0; width: 0; height: 0;}");
            client.print(".slider {position: absolute; cursor: pointer; top: -10; left: 0; right: 0; bottom: 10; background-color: #ccc; transition: .4s;}");
            client.print(".slider:before {position: absolute; content: \"\"; height: 16px; width: 16px; left: 2px; bottom: 2px; background-color: white; transition: .4s;}");
            client.print("input:checked + .slider {background-color:#AAAAAA;}");
            client.print("input:focus + .slider {box-shadow: 0 0 1px #AAAAAA;}");
            client.print("input:checked + .slider:before {-webkit-transform: translateX(20px);}");
            client.print(".slider.round {border-radius: 34px;}");
            client.print(".slider.round:before {border-radius: 50%;}");
            
            client.print("</style>");
            client.print("</head>");
            client.print("<body>");
            client.print("<div id=\"Page1\">");
            client.print("The time is ");
            client.print(day_of_the_week);
            client.print(" - ");
            client.print(time_now_2);
            client.println(" and we read the following values:");
            client.print("<br>");
            client.println("<br />");
            client.print("The soil humidity is: ");
            client.print(soil_humidity);
            client.print("<progress id=\"file\" value=\"");
            client.print(soil_humidity);
            client.print("\" max=\"100\"> </progress>");
            client.print(" and the digital output is: ");
            client.print(soil_humidity_state);
            if (soil_humidity_state == false)
            {
              client.print(" - the soil seems pretty wet to me.");
              client.print("<img style=\"vertical-align:middle\" src=\"https://img.icons8.com/ios-glyphs/30/000000/soil.png\" width = 40px />");
            }
            else
            {
              client.print(" - the soil seems pretty dry to me.");
              client.print("<img style=\"vertical-align:middle\" src=\"https://img.icons8.com/ios/50/000000/soil.png\" width = 40px />");
            }
            client.println("<br />");
            client.println("<br />");
            client.print("Air humidity is: ");
            client.print(air_humidity);
            client.print(",temperature is: ");
            client.print(temperature);
            client.print(" (C),and the heat index is: ");
            client.print(heat_index_celsius);
            client.println(" (C).");
            client.println("<br />");
            client.print("For our Avocado plant: ");
            if ((25<heat_index_celsius) && (heat_index_celsius<28))
            {
                client.print("The temperature is ideal for growth. "); 
            }
            else if (heat_index_celsius<10)
            {
                client.print("<p style=\"color:blue;\">It is dangerously cold for the plant!</p>");
            }
            else if (heat_index_celsius>35)
            {
                client.print("<p style=\"color:red;\">It is dangerously hot for the plant!</p>");
            }        
            else
            {            
                client.print("The ideal temperature for growth is between 25 and 28 degrees Celsius. Healthy trees can tolerate a big deviations from this margin. ");
            }
            if(air_humidity<60)
            {
              client.print("The best air humidity for ideal avocado growth is above 60%");
            }
            client.println("<br>");
            client.println("<button onclick=\"showGraph()\">Here's a helpfull table related to heat index</button>");
            client.println("<br />");
            client.print("<div id=\"graph\" style=\"display:none\">");
            client.print("<img src=\"https://www.researchgate.net/publication/325471848/figure/fig2/AS:632342498537474@1527773608374/Apparent-temperature-heat-index-in-degrees-Celsius-according-to-air-temperature-and.png\" width = 600px />");
            client.print("</div>");
            client.print("Light reading value is: ");
            client.print(light);
            if (light < 10) {
              client.println("-Dark");
              client.println("<img style=\"vertical-align:middle\" src =\"https://seekicon.com/free-icon-download/brightness-3_1.svg\" width= 40px/>");
            } else if (light < 200 ) {
              client.println("-Dim");
              client.println("<img style=\"vertical-align:middle\" src =\"https://seekicon.com/free-icon-download/brightness-2_2.svg\" width= 40px/>");
            } else if (light < 500 ) {
              client.println("-Light");
              client.println("<img style=\"vertical-align:middle\" src =\"https://seekicon.com/free-icon-download/brightness-alt-low_1.svg\" width= 40px/>");              
            } else if (light < 800 ) {
              client.println("-Bright");
              client.println("<img style=\"vertical-align:middle\" src =\"https://seekicon.com/free-icon-download/brightness-low_2.svg\" width= 40px/>"); 
            } else {
              client.println("-Very Bright");
              client.println("<img style=\"vertical-align:middle\" src =\"https://seekicon.com/free-icon-download/brightness-high_2.svg\" width= 40px/>"); 
            }
            client.println("<br>");
            client.println("<br>");
            client.print("Water level: ");
            client.print(waterlevel_value);
            client.print(", out of a maximum value of: ");
            client.print(maxwatervalue);
            client.print(", percentage level: ");
            client.print(waterpercent);
            client.println(". The calibrated maximum value is ");
            client.print(calibrated_max_water_level);
            client.print(".");
            client.println("<br />");
            if(maxwatervalue>calibrated_max_water_level)
            {
              if(maxwatervalue<calibrated_max_water_level*1.1)
              {
                client.println("The maximum recorded value is slightly higher than the value obtained during sensor calibration, however a deviation so small is to be expected. ");
              }
              else
              {
                client.println("The maximum recorded value is slightly higher than the value obtained during sensor calibration, with a big deviation. ");
                client.println("<p style=\"color:red;\">CONSIDER CLEANING OR REPLACING THE SENSOR</p>");            
              }
              client.println("<br>");
              client.println("<a href=\"/?Calibrate\"\">Update calibrated maximum value</a>");
              
            }
            client.println("<br /><br />");
            client.println("If you wish to change the ideal temperature values (Celsius) for the plant, submit here:");
            client.print("<form action=\"?SubmitTemp\">");
            client.print("<label for=\"Min\">Minimum:</label>");
            client.print("<input type=\"number\" name=\"MinTemp\" step=0.1 value=\"");
            client.print(min_plant_temp);
            client.print("\"><br>");
            client.print("<label for=\"Max\">Maximum:</label>");
            client.print("<input type=\"number\" name=\"MaxTemp\" step=0.1 value=\"");
            client.print(max_plant_temp);
            client.print("\"><br>");
            if(enable_fan==true)
            {            
            client.print("Enable fan functionality - Yes  <label class=\"switch\"><input type =\"checkbox\" name=\"Enable_fan\">");
            }
            else
            {
            client.print("Enable fan functionality - Yes  <label class=\"switch\"><input type =\"checkbox\" checked name=\"Enable_fan\">");
            }
            client.print("<span class=\"slider round\"></span></label>No<br>");
            client.print("<input type=\"submit\" value=\"Submit\">");
            client.print("</form><br>");       
            client.println("If you wish to change the Water Level that automatically triggers the Evacuation Pump, submit here");
            client.print("<form action=\"?SubmitLevel\">");
            client.print("<label for=\"WaterEvacLevel\">Water percentage level:</label>");
            client.print("<input type=\"number\" name=\"WaterEvacLevel\" min=\"0\" max=\"100\" size=\"3\" value=\"");
            client.print(evac_water_level);
            client.print("\">");
            client.print("<br><input type=\"submit\" value=\"Submit\">");
            client.print("</form>");
           
            client.print("<h1>Water Pump</h1>");
            client.print("<form action=\"?SubmitTime\">");
            client.print("<label for=\"Wquantity\">Time in seconds (between 0 and 400):</label>");
            client.print("<input type=\"number\" id=\"Wquantity\" name=\"Wquantity\" min=\"0\" max=\"400\" value=\"20\">");
            client.print("(0 = Indefinite time)");
            client.print("<br>");
            client.print("<input type=\"submit\" value=\"Start Pump\">");
            client.print("</form>");
            client.print("<form action=\"/?WaterPumpOff\">");
            client.print("<input type=\"number\" id=\"Aaa\" name=\"WaterPumpOff\" style=\"display:none\">");
            client.print("<input type=\"submit\" value=\"Stop Pump\"></form>");

            client.println("<h1>Evacuation Pump</h1>");
            client.print("<form action=\"?SubmitTime\">");
            client.print("<label for=\"Equantity\">Time in seconds (between 0 and 400):</label>");
            client.print("<input type=\"number\" id=\"Equantity\" name=\"Equantity\" min=\"0\" max=\"400\" value=\"20\">");
            client.print("(0 = Indefinite time)");
            client.print("<br>");
            client.print("<input type=\"submit\" value=\"Start Pump\">");
            client.print("</form>");
            client.print("<form action=\"/?EvacPumpOff\">");
            client.print("<input type=\"number\" id=\"Aaa\" name=\"EvacPumpOff\" style=\"display:none\">");
            client.print("<input type=\"submit\" value=\"Stop Pump\"></form>");
            //Checkbox 
            client.print("<form action=\"/?LogicType\">");
            if(weekly_or_sensor==false)
            {            
            client.print("Select logic functionality - Time-based <label class=\"switch\"><input type =\"checkbox\" name=\"week_sens\">");
            }
            else
            {
            client.print("Select logic functionality - Time-based <label class=\"switch\"><input type =\"checkbox\" checked name=\"week_sens\">");
            }
            client.print("<span class=\"slider round\"></span></label> Sensor Based<br>");
            client.print("<input type=\"submit\" value=\"Select\">");
            client.print("</form>");
            //checkbox end
            client.println("<br>");
            client.println("<a href=\"#\" onclick=\"return show('Page2','Page1','Page3','ConfirmPage');\",>Page 2 - Automatic time based watering</a>");
            client.println("<br>");
            client.println("<a href=\"#\" onclick=\"return show('Page3','Page1','Page2','ConfirmPage');\",>Page 3 - Automatic sensor+logic based watering</a>");
            client.print("</div>");
            client.print("<div id=\"Page2\" style=\"display:none\">");
            client.print("<form action=\"#\" onclick=\"return show('ConfirmPage','Page1','Page2','Page3') ><label for=\"Active_status\">Monday</label><br><label for=\"AutoWtrHT\">Start watering at hour:</label><input type=\"number\" id=\"AutoWtrHT\" name=\"0AutoWtrHT\" value=\"0\" min=\"0\" max=\"23\" size=\"2\"><label for=\"AutoWtrMT\">minute:</label><input type=\"number\" id=\"AutoWtrMT\" name=\"AutoWtrMT\" value=\"0\" min=\"0\" max=\"59\" size=\"2\"><label for=\"AutoWtrSD\">for duration (in seconds):</label><input type=\"number\" id=\"AutoWtrSD\" name=\"AutoWtrSD\" value=\"120\" min=\"1\" max=\"400\" size=\"3\"><br><input type=\"submit\" value=\"Submit\"></form>");
            for(int i = 0;i<5;i++)
            {
              if(auto_timed_watering[0][i]==true)
              {
                client.print("Monday schedule (click each to delete) : ");
                client.print("<br>");
                break;
              }     
            }
            for(int i = 0;i<5;i++)
            {
              if(auto_timed_watering[0][i]==true)
              {
                client.print(monday_auxiliary_href[i]);
                client.print(auto_timed_hour[0][i]);
                client.print(":");
                client.print(auto_timed_minute[0][i]);
                client.print(" - for ");
                client.print(auto_timed_duration[0][i]);
                client.print(" seconds.");
                client.print("</a><br>");
              }
            }
            client.print("<br>");
            client.print("<form action=\"#\"><label for=\"Active_status\">Tuesday</label><br><label for=\"1AutoWtrHT\">Start watering at hour:</label><input type=\"number\" id=\"AutoWtrHT\" name=\"1AutoWtrHT\" value=\"0\" min=\"0\" max=\"23\" size=\"2\"><label for=\"AutoWtrMT\">minute:</label><input type=\"number\" id=\"AutoWtrMT\" name=\"AutoWtrMT\" value=\"0\" min=\"0\" max=\"59\" size=\"2\"><label for=\"AutoWtrSD\">for duration (in seconds):</label><input type=\"number\" id=\"AutoWtrSD\" name=\"AutoWtrSD\" value=\"120\" min=\"1\" max=\"400\" size=\"3\"><br><input type=\"submit\" value=\"Submit\"></form>");
            for(int i = 0;i<5;i++)
            {
              if(auto_timed_watering[1][i]==true)
              {
                client.print("Tuesday schedule (click each to delete) : ");
                client.print("<br>");
                break;
              }
            }
                        for(int i = 0;i<5;i++)
            {
              if(auto_timed_watering[1][i]==true)
              {
                client.print(tuesday_auxiliary_href[i]);
                client.print(auto_timed_hour[1][i]);
                client.print(":");
                client.print(auto_timed_minute[1][i]);
                client.print(" - for ");
                client.print(auto_timed_duration[1][i]);
                client.print(" seconds.");
                client.print("</a><br>");
              }
            }
            client.print("<br>");
            client.print("<form action=\"#\"><label for=\"Active_status\">Wednesday</label><br><label for=\"2AutoWtrHT\">Start watering at hour:</label><input type=\"number\" id=\"AutoWtrHT\" name=\"2AutoWtrHT\" value=\"0\" min=\"0\" max=\"23\" size=\"2\"><label for=\"AutoWtrMT\">minute:</label><input type=\"number\" id=\"AutoWtrMT\" name=\"AutoWtrMT\" value=\"0\" min=\"0\" max=\"59\" size=\"2\"><label for=\"AutoWtrSD\">for duration (in seconds):</label><input type=\"number\" id=\"AutoWtrSD\" name=\"AutoWtrSD\" value=\"120\" min=\"1\" max=\"400\" size=\"3\"><br><input type=\"submit\" value=\"Submit\"></form>");
            for(int i = 0;i<5;i++)
            {
              if(auto_timed_watering[2][i]==true)
              {
                client.print("Wednesday schedule (click each to delete) : ");
                client.print("<br>");
                break;
              }
            }
                        for(int i = 0;i<5;i++)
            {
              if(auto_timed_watering[2][i]==true)
              {
                client.print(wednesday_auxiliary_href[i]);
                client.print(auto_timed_hour[2][i]);
                client.print(":");
                client.print(auto_timed_minute[2][i]);
                client.print(" - for ");
                client.print(auto_timed_duration[2][i]);
                client.print(" seconds.");
                client.print("</a><br>");
              }
            }
            client.print("<br>");
            client.print("<form action=\"#\"><label for=\"Active_status\">Thursday</label><br><label for=\"3AutoWtrHT\">Start watering at hour:</label><input type=\"number\" id=\"AutoWtrHT\" name=\"3AutoWtrHT\" value=\"0\" min=\"0\" max=\"23\" size=\"2\"><label for=\"AutoWtrMT\">minute:</label><input type=\"number\" id=\"AutoWtrMT\" name=\"AutoWtrMT\" value=\"0\" min=\"0\" max=\"59\" size=\"2\"><label for=\"AutoWtrSD\">for duration (in seconds):</label><input type=\"number\" id=\"AutoWtrSD\" name=\"AutoWtrSD\" value=\"120\" min=\"1\" max=\"400\" size=\"3\"><br><input type=\"submit\" value=\"Submit\"></form>");
            for(int i = 0;i<5;i++)
            {
              if(auto_timed_watering[3][i]==true)
              {
                client.print("Thursday schedule (click each to delete) : ");
                client.print("<br>");
                break;
              }
            }
                        for(int i = 0;i<5;i++)
            {
              if(auto_timed_watering[3][i]==true)
              {
                client.print(thursday_auxiliary_href[i]);
                client.print(auto_timed_hour[3][i]);
                client.print(":");
                client.print(auto_timed_minute[3][i]);
                client.print(" - for ");
                client.print(auto_timed_duration[3][i]);
                client.print(" seconds.");
                client.print("</a><br>");
              }
            }
            client.print("<br>");
            client.print("<form action=\"#\"><label for=\"Active_status\">Friday</label><br><label for=\"4AutoWtrHT\">Start watering at hour:</label><input type=\"number\" id=\"AutoWtrHT\" name=\"4AutoWtrHT\" value=\"0\" min=\"0\" max=\"23\" size=\"2\"><label for=\"AutoWtrMT\">minute:</label><input type=\"number\" id=\"AutoWtrMT\" name=\"AutoWtrMT\" value=\"0\" min=\"0\" max=\"59\" size=\"2\"><label for=\"AutoWtrSD\">for duration (in seconds):</label><input type=\"number\" id=\"AutoWtrSD\" name=\"AutoWtrSD\" value=\"120\" min=\"1\" max=\"400\" size=\"3\"><br><input type=\"submit\" value=\"Submit\"></form>");
            for(int i = 0;i<5;i++)
            {
              if(auto_timed_watering[4][i]==true)
              {
                client.print("Friday schedule (click each to delete) : ");
                client.print("<br>");
                break;
              }
            }
                        for(int i = 0;i<5;i++)
            {
              if(auto_timed_watering[4][i]==true)
              {
                client.print(friday_auxiliary_href[i]);
                client.print(auto_timed_hour[4][i]);
                client.print(":");
                client.print(auto_timed_minute[4][i]);
                client.print(" - for ");
                client.print(auto_timed_duration[4][i]);
                client.print(" seconds.");
                client.print("</a><br>");
              }
            }
            client.print("<br>");
            client.print("<form action=\"#\"><label for=\"Active_status\">Saturday</label><br><label for=\"5AutoWtrHT\">Start watering at hour:</label><input type=\"number\" id=\"AutoWtrHT\" name=\"5AutoWtrHT\" value=\"0\" min=\"0\" max=\"23\" size=\"2\"><label for=\"AutoWtrMT\">minute:</label><input type=\"number\" id=\"AutoWtrMT\" name=\"AutoWtrMT\" value=\"0\" min=\"0\" max=\"59\" size=\"2\"><label for=\"AutoWtrSD\">for duration (in seconds):</label><input type=\"number\" id=\"AutoWtrSD\" name=\"AutoWtrSD\" value=\"120\" min=\"1\" max=\"400\" size=\"3\"><br><input type=\"submit\" value=\"Submit\"></form>");
            for(int i = 0;i<5;i++)
            {
              if(auto_timed_watering[1][i]==true)
              {
                client.print("Saturday schedule (click each to delete) : ");
                client.print("<br>");
                break;
              }
            }
                        for(int i = 0;i<5;i++)
            {
              if(auto_timed_watering[5][i]==true)
              {
                client.print(saturday_auxiliary_href[i]);
                client.print(auto_timed_hour[5][i]);
                client.print(":");
                client.print(auto_timed_minute[5][i]);
                client.print(" - for ");
                client.print(auto_timed_duration[5][i]);
                client.print(" seconds.");
                client.print("</a><br>");
              }
            }
            client.print("<br>");
            client.print("<form action=\"#\"><label for=\"Active_status\">Sunday</label><br><label for=\"6AutoWtrHT\">Start watering at hour:</label><input type=\"number\" id=\"AutoWtrHT\" name=\"6AutoWtrHT\" value=\"0\" min=\"0\" max=\"23\" size=\"2\"><label for=\"AutoWtrMT\">minute:</label><input type=\"number\" id=\"AutoWtrMT\" name=\"AutoWtrMT\" value=\"0\" min=\"0\" max=\"59\" size=\"2\"><label for=\"AutoWtrSD\">for duration (in seconds):</label><input type=\"number\" id=\"AutoWtrSD\" name=\"AutoWtrSD\" value=\"120\" min=\"1\" max=\"400\" size=\"3\"><br><input type=\"submit\" value=\"Submit\"></form>");
            for(int i = 0;i<5;i++)
            {
              if(auto_timed_watering[6][i]==true)
              {
                client.print("Sunday schedule (click each to delete) : ");
                client.print("<br>");
                break;
              }
            }
                        for(int i = 0;i<5;i++)
            {
              if(auto_timed_watering[6][i]==true)
              {
                client.print(sunday_auxiliary_href[i]);
                client.print(auto_timed_hour[6][i]);
                client.print(":");
                client.print(auto_timed_minute[6][i]);
                client.print(" - for ");
                client.print(auto_timed_duration[6][i]);
                client.print(" seconds.");
                client.print("</a><br>");
              }
            }
            client.println("<br />");
            client.println("<br />");
            client.println("<a href=\"#\" onclick=\"return show('Page1','Page2','Page3','ConfirmPage');\",>Page 1 - Sensor reading + Instantaneous pump action</a>");
            client.println("<br />");
            client.println("<a href=\"#\" onclick=\"return show('Page3','Page1','Page2','ConfirmPage');\",>Page 3 - Automatic sensor+logic based watering</a>");
            client.print("</div>");
            client.print("<div id=\"Page3\" style=\"display:none\">");
            if(sens_ana_dig==false)
            {            
            client.print("<form><br><br><br>Soil sensor selection: Analog  <label class=\"switch\"><input type =\"checkbox\" name=\"An_Dig\">");
            }
            else
            {
            client.print("<form><br><br><br>Soil sensor selection: Analog  <label class=\"switch\"><input type =\"checkbox\" checked name=\"An_Dig\">");
            }
            client.print("<span class=\"slider round\"></span></label>Digital<br>");
            client.print("<label>Desired Analog Soil Humidity Level=</label><input type=\"number\" name=\"HumLevA\" value =\"");
            client.print(sens_hum_trigger);
            client.print("\" min=\"0\" max=\"100\"size=\"3\">");
            client.print("<br><br><br>Dry Cycle Duration:<br><br>");
            client.print("<label>Days=</label><input type=\"number\" name=\"CycDryD\" value =\"");
            client.print(sens_dry_cyc_d);
            client.print("\" min=\"0\" max=\"99\" size=\"2\"><br>");
            client.print("<label>Hours=</label><input type=\"number\" name=\"CycDryH\" value =\"");
            client.print(sens_dry_cyc_h);
            client.print("\" min=\"0\" max=\"23\" size=\"2\"><br>");
            client.print("<label>Minutes=</label><input type=\"number\" name=\"CycDryM\" value =\"");
            client.print(sens_dry_cyc_m);
            client.print("\" min=\"0\" max=\"59\"size=\"2\"><br>");
            client.print("<label>Seconds= </label><input type=\"number\" name=\"CycDryS\" value =\"");
            client.print(sens_dry_cyc_s);
            client.print("\" min=\"0\" max=\"59\"size=\"2\"><br>");
            client.print("<br><br>Watering duration:<br><br>");
            client.print("<label>Base Time Duration=</label><input type=\"number\" name=\"BaseDur\" value =\"");
            client.print(sens_base_dur_s);
            client.print("\" min=\"0\" max=\"9999\" size=\"5\"><br><br><br>");
            client.print("<input type=\"submit\" value=\"Submit\"></form><br>");
            client.print("If the soil humidity level rises above the set level during the \"Dry Cycle\" (presume rain or manual watering/pump activation),<br>");
            client.print("the \"Cycle\" is abandoned, and the program waits for a new moment the humidity drops to below the set level.<br><br>If soil sensor is Analog:<br>");
            client.print("Total watering time = Base duration * [ 1 + (Soil Humidity trigger value {for triggering the Dry Cycle} - Soil Humidity level {at start time of Watering})]<br><br>");
            client.print("If soil sensor is Digital:<br>Total watering time = Base Duration<br><br>");
            
            client.println("<br />");
            client.println("<a href=\"#\" onclick=\"return show('Page1','Page2','Page3','ConfirmPage');\",>Page 1 - Sensor reading + Instantaneous pump action</a>");
            client.println("<br />");
            client.println("<a href=\"#\" onclick=\"return show('Page2','Page1','Page3','ConfirmPage');\",>Page 2 - Automatic time based watering</a>");
            client.print("</div>");
            client.print("</body>");
            client.print("<script>");
            client.print("function show(shown, hidden1, hidden2, hidden3) {");
            client.print("document.getElementById(shown).style.display='block';");
            client.print("document.getElementById(hidden1).style.display='none';");
            client.print("document.getElementById(hidden2).style.display='none';");
            client.print("document.getElementById(hidden3).style.display='none';");
            client.print("return false;}");
            client.print("function showGraph() {");
            client.print("var x = document.getElementById(\"graph\");");
            client.print("if (x.style.display === \"none\"){x.style.display = \"block\";}");
            client.print("else{x.style.display = \"none\";}}");
            //            client.println("setTimeout(function(){window.location.(192.168.0.110);},1000)");
            client.print("function onButtonClick(){ alert(");
            client.print(now.Second());
            client.print(")}");
            client.print("</script>");
            client.print("</html>");

            delay(100);

            if (readString.indexOf("?WaterPumpOff") > 0)
            {
              digitalWrite(PumpWater, HIGH);
              delay(10);
            }
            if (readString.indexOf("?EvacPumpOff") > 0)
            {
              digitalWrite(PumpEvac, HIGH);
              delay(10);
            }
            if (readString.indexOf("WaterEvacLevel") > 0)
            {
              evac_water_level = readString.substring(readString.indexOf("WaterEvacLevel=")+15, readString.length() - 9).toInt();
              delay(10);
            }
            if (readString.indexOf("MinTemp") > 0)
            {
              
              min_plant_temp = readString.substring(readString.indexOf("MinTemp=")+8, readString.indexOf("&MaxTemp=")).toFloat();
              if(readString.indexOf("Enable_fan=") > 0)
              {
                enable_fan = false;
                max_plant_temp = readString.substring(readString.indexOf("MaxTemp=")+8, readString.indexOf("&Enable_fan")).toFloat();
              }
              else
              {
                enable_fan = true;
                max_plant_temp = readString.substring(readString.indexOf("MaxTemp=")+8, readString.length() - 9).toFloat();
              }
              delay(10);
            }
            
            if (readString.indexOf("?Wquantity") > 0)
            {
              String aux_pump_duration = readString.substring(16, readString.length() - 11);
              pump_duration = aux_pump_duration.toInt();
              digitalWrite(PumpWater, LOW);
              if (pump_duration != 0)
              { 
                end_time_water_pump=calculateEndTime(now,pump_duration);
                pump_water_set_duration=true;              
              }            
              delay(10);
            }
            if (readString.indexOf("?Equantity") > 0)
            {
              String aux_pump_duration = readString.substring(16, readString.length() - 11);
              pump_duration = aux_pump_duration.toInt();
              digitalWrite(PumpEvac, LOW);
              if (pump_duration != 0)
              { 
                end_time_evac_pump=calculateEndTime(now,pump_duration);
                pump_evac_set_duration=true;              
              }            
              delay(10);
            }
            if (readString.indexOf("?Calibrate") > 0)
            {
              calibrated_max_water_level = maxwatervalue;
              waterpercent = map(waterlevel_value, 200, calibrated_max_water_level, 0, 100);
              waterpercent = max(waterpercent,0);
              delay(10);
            }
            if (readString.indexOf("AutoWtr") >0)
            {
              int d = readString.substring(readString.indexOf("AutoWtrHT=")-1,readString.indexOf("AutoWtrHT=")).toInt();
              int h = readString.substring(readString.indexOf("AutoWtrHT=")+10,readString.indexOf("&AutoWtrMT=")).toInt();
              int m = readString.substring(readString.indexOf("AutoWtrMT=")+10,readString.indexOf("&AutoWtrSD=")).toInt();
              int dur = readString.substring(readString.indexOf("AutoWtrSD=")+10,readString.length() - 11).toInt();
              RecordAutoWaterTime(d,h,m,dur);  
              delay(10);      
            }
             if (readString.indexOf("?Del") > 0)
            {
              int d = readString.substring(readString.indexOf("?Del")+4,readString.indexOf("?Del")+5).toInt();
              int t = readString.substring(readString.indexOf("?Del")+5,readString.indexOf("?Del")+6).toInt();         
              DeleteAutoWaterTime(d,t); 
              delay(10);  
            }
              if (readString.indexOf("HumLevA=") > 0)
            {
              
              sens_hum_trigger = readString.substring(readString.indexOf("HumLevA=")+8,readString.indexOf("&CycDryD=")).toInt();
              sens_dry_cyc_d = readString.substring(readString.indexOf("CycDryD=")+8,readString.indexOf("&CycDryH=")).toInt();
              sens_dry_cyc_h = readString.substring(readString.indexOf("CycDryH=")+8,readString.indexOf("&CycDryM=")).toInt();
              sens_dry_cyc_m = readString.substring(readString.indexOf("CycDryM=")+8,readString.indexOf("&CycDryS=")).toInt();
              sens_dry_cyc_s = readString.substring(readString.indexOf("CycDryS=")+8,readString.indexOf("&BaseDur=")).toInt();
              sens_base_dur_s = readString.substring(readString.indexOf("BaseDur=")+8,readString.length() - 11).toInt();

              delay(10);
              dry_cyc_active=false;
              wet_cyc_active=false;
              sensor_watering_state=false;

              if(readString.indexOf("?HumLevA=") > 0)
              {
                sens_ana_dig = false;
              }
              else
              {
                sens_ana_dig = true;
              }
              delay(10); 
            }
              if (readString.indexOf("? HTTP/1.1") > 0)
              {
                weekly_or_sensor = false;
                dry_cyc_active=false;
                wet_cyc_active=false;
                sensor_watering_state=false;
                digitalWrite(PumpWater,HIGH);
                digitalWrite(PumpEvac,HIGH);
                }
              if (readString.indexOf("?week_sens=on") > 0)
              {
                weekly_or_sensor = true;
                dry_cyc_active=false;
                wet_cyc_active=false;
                sensor_watering_state=false;
                digitalWrite(PumpWater,HIGH);
                digitalWrite(PumpEvac,HIGH);
              }
            
            readString = "";

            delay(10);
            client.stop();
            delay(10);
            Serial.println("client disonnected");
          }
        }
      }
    }
  }

}

//Functions

#define countof(a) (sizeof(a) / sizeof(a[0]))

// Date setup done on a previous test run of the module, it should remember the time because of the battery - if not - include setup for module in void setup
String printDateTime(const RtcDateTime& dt)
{
  char datestring[20];

  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
             dt.Month(),
             dt.Day(),
             dt.Year(),
             dt.Hour(),
             dt.Minute(),
             dt.Second() );
  return datestring;
}

void AutoWaterTime(RtcDateTime current_time){
  int current_day=int(current_time.DayOfWeek())-1; //RTC assigns days of week from 1->7 ; we use them in code (arrays) from 0->6
  int current_hour=int(current_time.Hour());
  int current_minute=int(current_time.Minute());

  for(int i=0;i<5;i++){
    if(auto_timed_hour[current_day][i]==current_hour){
      if((auto_timed_minute[current_day][i]==current_minute) && (current_time.Second()==0)){
        if(pump_water_scheduled_duration == false){
          digitalWrite(PumpWater,LOW);
          end_time_timed_Wpump = calculateEndTime(current_time,auto_timed_duration[current_day][i]);
          pump_water_scheduled_duration = true;
          }
        if(pump_evac_scheduled_duration == false){     
        digitalWrite(PumpEvac,LOW);
        int low_end_timer = min(20,auto_timed_duration[current_day][i]);
        end_time_timed_Epump = calculateEndTime(current_time,low_end_timer); //to avoid the water collection tray to overflow whenever we water the plant based on schedule, we also drain the tray for 20 seconds.
        pump_evac_scheduled_duration = true;
        }
      }
    }
  }
}


int RecordAutoWaterTime(int day,int hour,int minute,int duration){
//first - record the data for automatic timed watering into memory
  for(int i=0;i<5;i++){
    if(auto_timed_watering[day][i]==false){
      auto_timed_watering[day][i]=true;
      auto_timed_hour[day][i]=hour;
      auto_timed_minute[day][i]=minute;
      auto_timed_duration[day][i]=duration;
      break;
    }
  }
}

int DeleteAutoWaterTime(int day, int time_slot){
  auto_timed_watering[day][time_slot]=false;
  auto_timed_hour[day][time_slot]=24;
  auto_timed_minute[day][time_slot]=60;
  auto_timed_duration[day][time_slot]=0;
}
//Function is done, to be rechecked (testing is hard because must wait for real plant sensor values to be accurate) and place in code with values
int SensorDryCycle(RtcDateTime current_time, int s_humidity, bool s_humidity_state,bool s_ana_dig,int s_hum_trigger,int s_dry_cyc_d,int s_dry_cyc_h,int s_dry_cyc_m,int s_dry_cyc_s,int s_base_dur_s){
    if((dry_cyc_active==false) && (wet_cyc_active==false))
    {
      if(((s_ana_dig==false) && (s_humidity <= s_hum_trigger)) || ((s_ana_dig==true) && (s_humidity_state == true)))
      {
        dry_cyc_active = true;
        double tot_time_in_sec= s_dry_cyc_s + (s_dry_cyc_m * 60) + (s_dry_cyc_h * 3600) + (s_dry_cyc_d * 86400);
        DryCycEnd = calculateEndTime(current_time,tot_time_in_sec);
      } 
    }   
    if((dry_cyc_active==true)&& (wet_cyc_active==false)&& (sensor_watering_state==false))
    {
      if(((s_ana_dig==false) && (s_humidity > s_hum_trigger + 5)) || ((s_ana_dig==true) && (s_humidity_state == false)))
      {
        dry_cyc_active = false;
      }
      if (current_time >= DryCycEnd)
      {
        if(s_ana_dig==true)
        {
          sens_watering_time = s_base_dur_s;
        }
        else if(s_ana_dig==false)
        {
          sens_watering_time = s_base_dur_s * (1 + (s_hum_trigger - s_humidity)/100);        
        }
        sens_end_water_time = calculateEndTime(current_time,sens_watering_time);
        digitalWrite(PumpWater, LOW);
        sensor_watering_state=true;
      }
    }
    if((sensor_watering_state==true) && (current_time >= sens_end_water_time) && (wet_cyc_active==false)){        Serial.println("S-A TERMINAT UDAREA, incepe WET CYCLE");
      sensor_watering_state=false;
      digitalWrite(PumpWater, HIGH);
      double tot_time_in_sec= s_dry_cyc_s + (s_dry_cyc_m * 60) + (s_dry_cyc_h * 3600) + (s_dry_cyc_d * 86400);
      WetCycEnd = calculateEndTime(current_time,tot_time_in_sec);
      wet_cyc_active=true;
      dry_cyc_active = false;   
    }
    if((current_time >= WetCycEnd) && (wet_cyc_active==true))
    wet_cyc_active = false;
    }

RtcDateTime calculateEndTime(const RtcDateTime& dt, int duration_in_seconds)
{
  int day_limitor = 32; //limitor is maximum value that the clock can take in that spot + 1 ; (ex:for seconds and minutes -> 59 + 1 = 60, used below directly - all limitors except the day limitor are constant)
  //day_limitor is dependend on current month, and in the case of february, on status of leap year - calculated below
  switch (int(dt.Month()))
  {
    case 1: case 3: case 5: case 7: case 8: case 10: case 12:
      day_limitor = 32;
      break;
    case 4: case 6: case 9: case 11:
      day_limitor = 31;
      break;
    case 2:
      if (int(dt.Year()) / 4 != 0)
      {
        day_limitor = 29;
      }
      else if (int(dt.Year()) / 100 != 0)
      {
        day_limitor = 30;
      }
      else if (int(dt.Year()) / 400 != 0)
      {
        day_limitor = 29;
      }
      else
      {
        day_limitor = 30;
      }
      break;
    default: //we already cover all possible months so we should never reach default state, but for safety I'll define it anyway
      day_limitor = 32;
      break;
  }
  //for day it needs to be calculated more precisely because diferent months have diferent day counts, and february fluctuates in leap-years.

  uint8_t end_time_second = (int(dt.Second()) + duration_in_seconds) % 60;
  uint8_t end_time_minute = (int(dt.Minute()) + (int(dt.Second()) + duration_in_seconds) / 60) % 60;
  uint8_t end_time_hour   = (int(dt.Hour())   + (int(dt.Minute()) + (int(dt.Second()) + duration_in_seconds) / 60) / 60) % 24;
  uint8_t end_time_day    = (int(dt.Day())    + (int(dt.Hour())   + (int(dt.Minute()) + (int(dt.Second()) + duration_in_seconds) / 60) / 60) / 24) % day_limitor;
  uint8_t end_time_month  = (int(dt.Month())  + (int(dt.Day())    + (int(dt.Hour())   + (int(dt.Minute()) + (int(dt.Second()) + duration_in_seconds) / 60) / 60) / 24) / day_limitor) % 13;
  uint16_t end_time_year   =  int(dt.Year())   + (int(dt.Month())  + (int(dt.Day())    + (int(dt.Hour())   + (int(dt.Minute()) + (int(dt.Second()) + duration_in_seconds) / 60) / 60) / 24) / day_limitor) / 13; //We have no limit for numbering years, this part does not need a modulo operator for carryover

  return RtcDateTime(end_time_year, end_time_month, end_time_day, end_time_hour, end_time_minute, end_time_second);

}
