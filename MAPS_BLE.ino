#include <Time.h>            // time library
#include "DHT.h"             // temperature and humidity DHT22 library
#include <Wire.h>            // Wire library
#include <TimerOne.h>        // timerone library, we need this to trigger CO2 sensor
#include <SoftwareSerial.h>  // SoftwareSerial library, we need this for Grove BLE v1.0
#include <SFE_BMP180.h>      // bmp180
#include "RTClib.h"          // tiny rtc i2c module

#define ERROR -999

#define DHTPIN 2              // DHT pin
#define DHTTYPE DHT22         // DHT type
#define RxD 6                 // connect to BLE tx
#define TxD 7                 // connect to BLE rx
#define pmRxD 4                 // connect to pm2.5 G3 tx
#define pmTxD 5                 // connect to pm2.5 G3 rx

#define source_drive 9        // trigger CO2 sensor
#define CALIBRATE_CONCENTRATION 1000
#define CO2_a 0.0007
#define CO2_b 0.00041
#define CO2_c 0.897
#define CO2_Ba 0.0005
#define CO2_CLB_RATIO 1.269052
#define CO2_ZERO 1.354519
#define CO2_Tcal 26.74031

#define MAPS_ID "000"

#define SAFE_LIGHT 10  //led pin #
#define WARN_LIGHT 11
#define DANGER_LIGHT 12

#define PM_SAFE 35
#define PM_WARN 53
#define CO2_SAFE 1000
//
//#define CO2_CLB_RATIO 1.232713
//#define CO2_ZERO 1.385105
//#define CO2_Tcal 26.512873

#define analog_vol 0.0048875  // 5v/1023


SoftwareSerial BLE(RxD,TxD);  // for BLE


SoftwareSerial G3(pmRxD,pmTxD);       // for pm2.5 G3 module
/********
for CO2 temperature and Resistor transform, these values are from CO2 spec
********/
const int Z = 0;
const int T = 10;
const int TWT = 20;
const int THT = 30;
const int FT = 40;
const int FFT = 50;
const int ST = 60;

const int Z_R = 8661;
const int T_R = 5541;
const int TWT_R = 3655;
const int THT_R = 2478;
const int FT_R = 1723;
const int FFT_R = 1225;
const int ST_R = 889;



/********
BLE tx string
********/
char buff[150];

/********
DHT 22 sensor
********/
DHT dht(DHTPIN, DHTTYPE);   // get DHT ready
float DHT_temperature = 0;  // set global temperature
float DHT_humidity = 0;     // set global humidity

/********
CO2 sensor
********/
float ratioResult = 0;      // ACT and REF voltage ratio
float CO2_temperatureResult = 0;// CO2 sensor's temperature  

/********
PM2.5 sensors
********/
float pm1_result = 0;
float pm25_result = 0;      // G3 sensor value
float pm10_result = 0;      // G3 sensor value

/********
Barometer BMP180
********/
SFE_BMP180 pressure;        // barometer 
double barometer_value;     // barometer value

/********
get BLE ready
********/
void setupBleConnection()
{
	BLE.begin(9600);        // Set BLE BaudRate to default baud rate 9600
	BLE.print("AT+CLEAR");  // clear all previous setting
	BLE.print("AT+ROLE0");  // set the bluetooth name as a slaver
	BLE.print("AT+SAVE1");  // don't save the connect information
}

/********
RTC
********/
RTC_DS1307 rtc;

/********
CPU tick
********/
unsigned long time_after_booting = 0;

/********
package sent
********/
unsigned long packages_num = 0;

/********
onboard led
********/
int led = 13;

/*
   sssssss   eeeeee  ttttttt  u    u    pppppp
   s         e          t     u    u    p    p
   sssssss   eeeeee     t     u    u    pppppp
         s   e          t     u    u    p
   sssssss   eeeeee     t     uuuuuu    p
*/


void setup(){
	Serial.begin(9600);
	
	// BLE pin, set pin 6 as arduino rx, pin 7 as arduino tx, with Software serial library, and start BLE
	pinMode(RxD, INPUT);
	pinMode(TxD, OUTPUT);
	setupBleConnection(); 
	
	
	// G3 pin, set pin 3 as arduino rx, pin 4 as arduino tx, with Software serial library, and start G3
	pinMode(pmRxD, INPUT);
	pinMode(pmTxD, OUTPUT);
	
	
	
	//for CO2 sensor, setup pwm on pin 9, 50% duty cycle
	pinMode(source_drive, OUTPUT);
	Timer1.initialize(1000000); 
	Timer1.pwm(9, 512, 400000);
	
	
	// DHT temperature and humidity
	dht.begin();
	
	
	// Barometer bmp180
	//  pressure.begin();
	
	// Check rtc, and set the time if it is not running
	if (! rtc.begin()) {
		Serial.println("Couldn't find RTC");
		while (1);
	}
	//  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
	//  if (! rtc.isrunning()) {
	//    Serial.println("RTC is NOT running!");
	//    // Following line sets the RTC to the date & time this sketch was compiled
	rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
	//    // This line sets the RTC with an explicit date & time, for example to set
	//    // Oct 16, 2015 at 3pm you would call:
	//    // rtc.adjust(DateTime(2015, 10, 16, 15, 0, 0));
	//  }
	
	// set led
	pinMode(SAFE_LIGHT, OUTPUT);
	digitalWrite(SAFE_LIGHT, HIGH);
	pinMode(WARN_LIGHT, OUTPUT);
	digitalWrite(WARN_LIGHT, HIGH);
	pinMode(DANGER_LIGHT, OUTPUT);
	digitalWrite(DANGER_LIGHT, HIGH);
	
	// set onboard led
	pinMode(led, OUTPUT); 
	digitalWrite(led, LOW);
}


/*
   L         OOOOOO   OOOOOO   pppppp
   L         O    O   O    O   p    p
   L         O    O   O    O   pppppp
   L         O    O   O    O   p
   LLLLLLL   OOOOOO   OOOOOO   p
*/
void loop(){
	// buffer for data value write through BLE
	char DHT_temp_buff[20];     // temperature value buffer
	char DHT_humidity_buff[20]; // humidity value buffer
	char pm_buff[20];           // pm2.5 value buffer
	char pressure[20];          // barometer value buffer
	char CO2_buff[20];
	
	double ABSx = 0;
	double SPANcal = 0;
	double ABS = 0;
	double ABSt = 0;
	double SPANt = 0;
	double CO2_ppm = 0;
	 
	// get CPU tick (second)
	time_after_booting = millis()/1000;
	
	// get time
	DateTime maps_time = rtc.now();
	
	// Reading alpha sense CO2 voltage, parameter: sensing time in millisecond
	ReadVoltage(57000);
	
	
	//try to get the concentration in ppm
	ABSx = Absorbance(CO2_CLB_RATIO, CO2_ZERO);
	SPANcal = Span(ABSx, CO2_b, CALIBRATE_CONCENTRATION, CO2_c);
	ABS = Absorbance(ratioResult, CO2_ZERO);
	ABSt = AbsorbanceCompensation(ABS, CO2_a, CO2_temperatureResult, CO2_Tcal);
	SPANt = SpanCompensation(SPANcal, CO2_Ba, CO2_temperatureResult, CO2_Tcal);
	CO2_ppm = GasConcentration(ABSt, SPANt, CO2_b, CO2_c, CO2_temperatureResult, CO2_Tcal);
	
	Serial.print("T+");
	Serial.print(maps_time.year(), DEC);
	Serial.print('+');
	Serial.print(maps_time.month(), DEC);
	Serial.print('+');
	Serial.print(maps_time.day(), DEC);
	Serial.print("+");
	Serial.print(maps_time.hour(), DEC);
	Serial.print('+');
	Serial.print(maps_time.minute(), DEC);
	Serial.print('+');
	Serial.print(maps_time.second(), DEC);
	Serial.print('+');
	Serial.print("C+");
	Serial.print(ratioResult, 6);
	Serial.print("+");
	Serial.print(CO2_temperatureResult, 6);
	Serial.print("+");
	Serial.print(CO2_ppm);
	Serial.print("+");
	
	// read temperature and humidity from DHT
	TmpHmd();
	Serial.print("D+");
	Serial.print(DHT_temperature);
	Serial.print("+");
	Serial.print(DHT_humidity);
	Serial.print("+");
	
	// PM2.5 
	pm25sensorG3(pm1_result, pm25_result, pm10_result);
	Serial.print("P+");
	Serial.print(pm1_result);
	Serial.print("+");
	Serial.print(pm25_result);
	Serial.print("+");
	Serial.print(pm10_result);
	Serial.print("+");
	
	// get barometer pressure value
	barometer_value = -1; // getPressure();
	Serial.print("B+");
	Serial.print(barometer_value);
	Serial.print("+");
	
	// get CPU tick
	Serial.print("U+");
	Serial.print(time_after_booting);
	Serial.print("+");
	
	// get package sequence
	Serial.print("K+");
	Serial.print(packages_num);
	packages_num+=1;
	Serial.print("+");
	
	// get MAPS ID
	Serial.print("I+");
	Serial.print(MAPS_ID);
	
	
	// get float value to string, and send by BLE, sprintf is not support for %f on Arduino due to some performance issue (from google XD)
	BLE.listen();
	dtostrf(DHT_temperature, 2, 2, DHT_temp_buff);
	dtostrf(DHT_humidity, 2, 2, DHT_humidity_buff);
	dtostrf(pm25_result, 2, 2, pm_buff);
	dtostrf(CO2_ppm, 2, 2, CO2_buff);
	
	sprintf(buff, "tmp,%s", DHT_temp_buff);
	BLE.print(buff);
	delay(250);
	
	sprintf(buff, "hmd,%s", DHT_humidity_buff);
	BLE.print(buff);
	delay(250);
	
	sprintf(buff, "pm25,%s", pm_buff);
	BLE.print(buff);
	delay(250);
	
	sprintf(buff, "co2,%s", CO2_buff);
	BLE.print(buff);
	
	// set led light state
	if(pm25_result <= PM_SAFE && CO2_ppm <= CO2_SAFE){  //both value are in safe state, green light
		digitalWrite(SAFE_LIGHT, HIGH);
		digitalWrite(WARN_LIGHT, LOW);
		digitalWrite(DANGER_LIGHT, LOW);
	}
	else if(pm25_result > PM_SAFE && pm25_result <= PM_WARN){  // pm2.5 are in the warning state, yellow light
		digitalWrite(SAFE_LIGHT, LOW);
		digitalWrite(WARN_LIGHT, HIGH);
		digitalWrite(DANGER_LIGHT, LOW);
	}
	else if(pm25_result > PM_WARN || CO2_ppm > CO2_SAFE){  // one of the value is over the limit, red light
		digitalWrite(SAFE_LIGHT, LOW);
		digitalWrite(WARN_LIGHT, LOW);
		digitalWrite(DANGER_LIGHT, HIGH);
	}
	else{
		// keep the same state
	}
	//  while(true){
	//    if(Serial.available()){
	//      if(Serial.read() == 'Y'){
	//        digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
	//        delay(250);               // wait for a second
	//        digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
	//        delay(250);
	//        digitalWrite(led, HIGH);
	//        delay(250);               
	//        digitalWrite(led, LOW);    
	//        delay(250);
	//        digitalWrite(led, HIGH);   
	//        delay(250);               
	//        digitalWrite(led, LOW);    
	//        delay(250);
	//        digitalWrite(led, HIGH);   
	//        delay(250);               
	//        digitalWrite(led, LOW); 
	//        delay(250);
	//        digitalWrite(led, HIGH);
	//        delay(250);               
	//        digitalWrite(led, LOW);    
	//        delay(250);
	//        digitalWrite(led, HIGH);   
	//        delay(250);               
	//        digitalWrite(led, LOW);  
	//        delay(250);
	//      }
	//      else{
	//        digitalWrite(led, HIGH);   
	//        delay(3000);               
	//        digitalWrite(led, LOW);    
	//      }
	//      break; 
	//    }
	//    
	//  }

}



/********
CO2 sensor's temperature, use Linear interpolation method
high = high temperature, low = low temperature, high_R = resistance value @ high, low_R = resistance value @ low, R2 = current resistance value
input: high, low temperature & resistance value and current resistance value
output: CO2 sensor's temperature
********/
float CO2_Temperature(float high, float low, float high_R, float low_R, float R2){
	float temperature = 0;
	temperature = (((high * R2) + (high_R * low) - (low * R2) - (high * low_R)) / (high_R - low_R));
	return temperature;
}

/********
CO2 sensor's temperature, need to read CO2 spec, use thermal resistance
input: current resistance value
output: CO2 sensor temperature
********/
float Resis2Temp(float R2){
	float temperature = 0;
	if(R2 <= Z_R && R2 > T_R){
	temperature = CO2_Temperature(T, Z, T_R, Z_R, R2); 
	}
	else if(R2 <= T_R && R2 > TWT_R){
	temperature = CO2_Temperature(TWT, T, TWT_R, T_R, R2);
	}
	else if(R2 <= TWT_R && R2 > THT_R){
	temperature = CO2_Temperature(THT, TWT, THT_R, TWT_R, R2);
	}
	else if(R2 <= THT_R && R2 > FT_R){
	temperature = CO2_Temperature(FT, THT, FT_R, THT_R, R2);
	}
	else if(R2 <= FT_R && R2 > FFT_R){
	temperature = CO2_Temperature(FFT, FT, FFT_R, FT_R, R2);
	}
	else if(R2 <= FFT_R && R2 > ST_R){
	temperature = CO2_Temperature(ST, FFT, ST_R, FFT_R, R2);
	}
	else{
	temperature = ERROR; //error :(
	}
	return temperature;
  
}
         


/********
CO2 sensor's voltage reading
we read Active, Reference voltage and resistance value
input: sensing time in millisecond
output: 
********/
void ReadVoltage(unsigned int sensingSecond){

	float tempActVol = 0;
	float tempRefVol = 0;
	float tempResisVol = 0;
	
	float sumRatioVol = 0;
	float sumResisVol = 0;
	
	float currentMaxActVol = 0;
	float currentMaxRefVol = 0;
	
	
	float ratioNum = 0;
	float resNum = 0;
	
	
	unsigned long now;
	unsigned long startTime = millis();
	
	
	//Read voltage for 'sensingSecond'
	while(1){
		now = millis();
		tempActVol = analogRead(A0);  // Act
		tempRefVol = analogRead(A1);  // Ref
		tempResisVol = analogRead(A2);// Resistance
		
		// to measure the current resistance for CO2 sensor it self, I learn from these place: 
		// http://forum.arduino.cc/index.php/topic,+21614.0.html
		// https://en.wikipedia.org/wiki/Voltage_divider
		// for R1, I use 10k Ohm, and Vin 5V
		tempResisVol = tempResisVol * analog_vol;
		sumResisVol += Resis2Temp(((10.0/((5.0 / tempResisVol)-1.0))*1000.0));
		resNum += 1;
		/*
		   CO2 voltage (both Act and Ref) is a wave, ex:
		   0, 0.45, 0.9, localMax, 1.0, 0.5, 0,.....,0, 0.43, 0.79, localMax, 1.1, 0.7, 0,......
		   we only need the localMax value, the code below is doing this job, I use Act voltage as the main target
		*/
		if(tempActVol > currentMaxActVol){
			currentMaxActVol = tempActVol;
			currentMaxRefVol = tempRefVol;
		}
		else if(tempActVol == 0 && currentMaxActVol != 0 && currentMaxRefVol != 0){
			sumRatioVol += (currentMaxActVol/currentMaxRefVol);
			ratioNum += 1;
			currentMaxActVol = 0;
			currentMaxRefVol = 0;
		}
		else{
			//nothing
		}
		if(now - startTime >= sensingSecond){
			break;
		}
	}  
	
	if(ratioNum > 0){
		ratioResult = sumRatioVol/ratioNum;
	}
	else{
		ratioResult = ERROR;
	}
	if(resNum > 0){
		CO2_temperatureResult = sumResisVol/resNum;
	}
	else{
		CO2_temperatureResult = ERROR;
	}

  

}      


/********
CO2 concentration (ppm) converter
********/ 
double Span(double ABSx, double b, double x, double c){
	double SPAN;
	double temp_denominator;
	double temp_xc;
	temp_xc = pow(x,c);
	temp_denominator = (1-exp(-(b * temp_xc)));
	SPAN = (ABSx/temp_denominator);
	return SPAN;
}
double Absorbance(double RATIO, double ZERO){
    double ABS;
    ABS = (1 - (RATIO/ZERO));
    return ABS;
}
double AbsorbanceCompensation(double ABS, double a, double T, double Tcal){
    double ABSt;
    ABSt = (1 - ((1 - ABS) * (1 + a * (T - Tcal))));
    return ABSt;
}
double SpanCompensation(double SPANcal, double Ba, double T, double Tcal){
    double SPANt;
    SPANt = (SPANcal + (Ba * (T - Tcal)));
    return SPANt;
}
double GasConcentration(double ABSt, double SPANt, double b, double c, double T, double Tcal){
    double temperature_para;
    double numerator;
    double denominator;
    double base;
    double main_para;
    double pow_num;
    double ppm;

    temperature_para = (T / Tcal);
    numerator = log( (1-(ABSt / SPANt)) );
    denominator = 0-b;
    base = (numerator / denominator);
    pow_num = (1 / c);
    main_para = pow(base, pow_num);
    ppm = temperature_para * main_para;
    return ppm;
}





/*==========Temperature and Humidity sensor function==========*/
void TmpHmd(){
	DHT_temperature = dht.readTemperature();
	DHT_humidity = dht.readHumidity();
}

/*==========PM2.5 function==========*/
void pm25sensorG3(float &pm1, float &pm25, float &pm10){
	int count=0;
	float pm25result = 0;
	byte incomeByte[24];
	boolean startcount=false;
	byte data;
	G3.begin(9600);
	while (1){
		if(G3.available()){
			data=G3.read();
			if(data==0x42 && !startcount){
		    	startcount = true;
		    	incomeByte[count]=data;
		    	count++;
		  	}
			else if(startcount){
				incomeByte[count]=data;
			    count++;
			    if(count>=24){
					break;
				}
			}
		}
	}
	G3.end();
	G3.flush();
	unsigned int calcsum = 0;
	unsigned int exptsum;
	// reference https://github.com/avaldebe/AQmon/blob/master/Documents/PMS3003_LOGOELE.pdf
	for(int i = 0; i < 22; i++) {
		calcsum += (unsigned int)incomeByte[i];
	}
	exptsum = ((unsigned int)incomeByte[22] << 8) + (unsigned int)incomeByte[23];
	if(calcsum == exptsum){
		pm1 = ((unsigned int)incomeByte[10] << 8) + (unsigned int)incomeByte[11];
		pm25 = ((unsigned int)incomeByte[12] << 8) + (unsigned int)incomeByte[13];
		pm10 = ((unsigned int)incomeByte[14] << 8) + (unsigned int)incomeByte[15];
	} 
	else{
	//    Serial.println("#[exception] PM2.5 Sensor CHECKSUM ERROR!");
	}     
}

/*==========BMP180 Atmosphere pressure function==========*/
double getPressure()
{
  char status;
  double T,P,p0,a;

  // You must first get a temperature measurement to perform a pressure reading.
  // Start a temperature measurement:
  // If request is successful, the number of ms to wait is returned.
  // If request is unsuccessful, 0 is returned.

  status = pressure.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:

    delay(status);

    // Retrieve the completed temperature measurement:
    // Note that the measurement is stored in the variable T.
    // Use '&T' to provide the address of T to the function.
    // Function returns 1 if successful, 0 if failure.

    status = pressure.getTemperature(T);
    if (status != 0)
    {
      // Start a pressure measurement:
      // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
      // If request is successful, the number of ms to wait is returned.
      // If request is unsuccessful, 0 is returned.

      status = pressure.startPressure(3);
      if (status != 0)
      {
        // Wait for the measurement to complete:
        delay(status);

        // Retrieve the completed pressure measurement:
        // Note that the measurement is stored in the variable P.
        // Use '&P' to provide the address of P.
        // Note also that the function requires the previous temperature measurement (T).
        // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
        // Function returns 1 if successful, 0 if failure.

        status = pressure.getPressure(P,T);
        if (status != 0)
        {
          return(P);
        }
        else{
//          Serial.println("error retrieving pressure measurement\n");
        } 
      }
      else{
//        Serial.println("error starting pressure measurement\n");
      } 
    }
    else{
//      Serial.println("error retrieving temperature measurement\n");
    }
  }
  else{
//    Serial.println("error starting temperature measurement\n");
  }
}
