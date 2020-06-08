
#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>

String apiKey = "O8ZZ2MX4GW2Z7F3Y";                  //  Enter your Write API key from ThingSpeak
const char *ssid =  "Ashok";                                    // replace with your wifi ssid and wpa2 key
const char *pass =  "Project2020";
const char* server = "api.thingspeak.com";
WiFiClient client;

#define ONE_WIRE_BUS 32                         //water temperature data pin to esp pin
#define DHTTYPE DHT22
#define DHTPIN 33                               //DHT22 data pin to esp pin
//#define EC_PIN 25//ec data pin to esp pin
#define SensorPin 26 //pH data pin to esp pin
#define Offset 0.2 //deviation compensate
#define samplingInterval 20
#define printInterval 800
#define ArrayLenth 40 //times of collection

#define TdsSensorPin 25 //EC data pin to esp pin
#define VREF 5.0      // analog reference voltage(Volt) of the ADC
#define SCOUNT  30           // sum of sample point
int analogBuffer[SCOUNT];    // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0,copyIndex = 0;
float averageVoltage = 0,tdsValue = 0,temperature = 25;
float voltage;
DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float Celcius=0;
float Fahrenheit=0;
int sensorValue = 0; 
unsigned long int avgValue; 
float b;
float pHValue;
int pHArray[ArrayLenth]; //Store the average value of the sensor feedback
int pHArrayIndex=0;
float h,t;

void setup() 
{
    Serial.begin(115200);
    delay(10);
    dht.begin();
    sensors.begin();
    Serial.println("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    
 
}
void loop() 
{
        EC_value();
        pH_value();
        water_temp();
        dht22();

 if (client.connect(server,80))                                 //   "184.106.153.149" or api.thingspeak.com
    {  
                            
    String postStr = apiKey;
    postStr +="&field1=";
    postStr += String(t);
    postStr += "&field2=";
    postStr += String(h);
    postStr +="&field3=";
    postStr += String(tdsValue);
    postStr += "&field4=";
    postStr += String(Celcius);
    postStr += "&field5=";
    postStr += String(pHValue);
    postStr += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
 
    Serial.print("Temp: ");
    Serial.println(t);
    Serial.println(" C"); 
    Serial.print("Humidity: ");
    Serial.println(h);
    Serial.print("EC:");
    Serial.print(tdsValue);
    Serial.print("WT:");
    Serial.print(Celcius);
    Serial.print("pH:");
    Serial.print(pHValue);
                             
    Serial.println("%. Send to Thingspeak.");
}
    client.stop();
    Serial.println("Waiting...");
    delay(10000);
}


String dht22()
{ 
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  //float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) )
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return "None";
  }

 Serial.print(F("Humidity: "));
 Serial.println(h);
 Serial.print(F("Temperature: "));
 Serial.println(t);
 return String(h);
 return String(t);
 
}




String EC_value() 
{
  static unsigned long analogSampleTimepoint = millis();
   if(millis()-analogSampleTimepoint > 40U)     //every 40 milliseconds,read the analog value from the ADC
   {
     analogSampleTimepoint = millis();
     analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    //read the analog value and store into the buffer
     analogBufferIndex++;
     if(analogBufferIndex == SCOUNT) 
         analogBufferIndex = 0;
   }   
   static unsigned long printTimepoint = millis();
   if(millis()-printTimepoint > 800U)
   {
      printTimepoint = millis();
      for(copyIndex=0;copyIndex<SCOUNT;copyIndex++)
        analogBufferTemp[copyIndex]= analogBuffer[copyIndex];
      averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 4096.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      float compensationCoefficient=1.0+0.02*(temperature-25.0);    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
      float compensationVolatge=averageVoltage/compensationCoefficient;  //temperature compensation
      tdsValue=(133.42*compensationVolatge*compensationVolatge*compensationVolatge - 255.86*compensationVolatge*compensationVolatge + 857.39*compensationVolatge)*0.5; //convert voltage value to tds value
      //Serial.print("voltage:");
      //Serial.print(averageVoltage,2);
      //Serial.print("V   ");
      Serial.print("TDS Value:");
      Serial.print(tdsValue,0);
      Serial.println("ppm");
      return String(tdsValue);
   }
}
int getMedianNum(int bArray[], int iFilterLen) 
{
      int bTab[iFilterLen];
      for (byte i = 0; i<iFilterLen; i++)
      bTab[i] = bArray[i];
      int i, j, bTemp;
      for (j = 0; j < iFilterLen - 1; j++) 
      {
      for (i = 0; i < iFilterLen - j - 1; i++) 
          {
        if (bTab[i] > bTab[i + 1]) 
            {
        bTemp = bTab[i];
            bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
         }
      }
      }
      if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
      else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
      return bTemp;
}
  
  
    


float water_temp()
{
   sensors.requestTemperatures();
  Celcius=sensors.getTempCByIndex(0);
  Fahrenheit=sensors.toFahrenheit(Celcius);
  return float(Celcius);
}






float pH_value()
{
  static unsigned long samplingTime = millis();
  static unsigned long printTime = millis();
  static float pHValue,voltage;
  if(millis()-samplingTime > samplingInterval)
  {
    pHArray[pHArrayIndex++]=analogRead(SensorPin);
    if(pHArrayIndex==ArrayLenth)pHArrayIndex=0;
    voltage = avergearray(pHArray, ArrayLenth)*3.3/4096;
    pHValue = 3.5*voltage+Offset;
    samplingTime=millis();
  }
  if(millis() - printTime > printInterval)
  { //Every 800 milliseconds, print a numerical, convert the state of the LED indicator
    Serial.print("Voltage:");
    Serial.print(voltage,2);
    Serial.print(" pH value: ");
    Serial.println(pHValue,2);
    printTime=millis();
  }
  }
  double avergearray(int* arr, int number){
  int i;
  int max,min;
  double avg;
  long amount=0;
  if(number<=0){
    Serial.println("Error number for the array to avraging!/n");
    return 0;
  }
  if(number<5){ //less than 5, calculated directly statistics
    for(i=0;i<number;i++){
      amount+=arr[i];
    }
    avg = amount/number;
    return avg;
  }
  else{
  if(arr[0]<arr[1]){
    min = arr[0];max=arr[1];
  }
  else{
    min=arr[1];max=arr[0];
  }
  for(i=2;i<number;i++){
    if(arr[i]<min){
      amount+=min; //arr<min
      min=arr[i];
    }
    else {
      if(arr[i]>max){
        amount+=max; //arr>max
        max=arr[i];
      }
      else{
         amount+=arr[i]; //min<=arr<=max
      }
    }
   }
   avg = (double)amount/(number-2);
  }
  return avg;
}
