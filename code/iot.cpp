#include <Wire.h>
#include <BH1750.h> 
#include <PMS.h>
#include <SoftwareSerial.h>
#include "Adafruit_BME680.h"
#include <ESP8266WiFi.h>

class MyData {
    public:
        float temperature;
        float humidity;
        float lux;
        float pm10;
        float pm25;
        float pm100;
        float iaq_index;
        int iaq_level;
	
	MyData(float temperature, float humidity, float lux, float pm10, float pm25, float pm100, float iaq_index, int iaq_level) {
		this->temperature = temperature;
		this->humidity = humidity;
		this->lux = lux;
		this->pm10 = pm10;
        this->pm25 = pm25;
		this->pm100 = pm100;
		this->iaq_index = iaq_index;
		this->iaq_level = iaq_level;
	}
};


char ssid[] = "nutt-guest";
char password[] = "plzplzplz";

char db_server[] = "nutt.live";
char thingspeak_server[] = "api.thingspeak.com";
char thingspeak_server_key[] = "PKQGREU4P2RR5DUI";

#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME680 bme;
BH1750 lightMeter;
SoftwareSerial pmsSerial(D3, D4);
WiFiClient client;

float pmat10 = 0;
float pmat25 = 0;
float pmat100 = 0;

float hum_score, gas_score;
float gas_reference = 250000;
float hum_reference = 40;
float Temperature = 0;
float Humidity = 0;
unsigned int IAQ_Level = 1;

void pms5003t_spec();
void connect_wifi();
void get_gas_reference();
void send_db(MyData data);
void send_thingspeak(MyData data);
void bme_init();
void bh1750_init();
void i2c_init();
unsigned int get_iaq_level(float iaq_index);

void setup() {
    Serial.begin(9600);
    pmsSerial.begin(9600);

    connect_wifi();
	i2c_init();
	bme_init();
	bh1750_init();
    get_gas_reference();
}

void loop() {
    float lux = lightMeter.readLightLevel();

    Serial.printf("Light: %f lux\n", lux);
    
	pms5003t_spec();

    Serial.printf("PM1.0= %f ug/m3 \nPM2.5= %f ug/m3 \nPM10= %f ug/m3 \n", pmat10, pmat25, pmat100);

	Temperature = bme.readTemperature();
    Serial.printf("Temperature = %f °C\n", Temperature);
    
	Humidity = bme.readHumidity();
    Serial.printf("Humidity = %f % \n", Humidity);

    float current_humidity = bme.readHumidity();
    if (current_humidity >= 38 && current_humidity <= 42) {
		hum_score = 0.25 * 100;
	} else {
        if (current_humidity < 38) {
			hum_score = 0.25 / hum_reference * current_humidity * 100;
		} else {
            hum_score = ((-0.25 / (100 - hum_reference) * current_humidity) + 0.416666) * 100;
        }
    }

    float gas_lower_limit = 5000;     // Bad air quality limit
    float gas_upper_limit = 50000;    // Good air quality limit
    if (gas_reference > gas_upper_limit) gas_reference = gas_upper_limit;
    if (gas_reference < gas_lower_limit) gas_reference = gas_lower_limit;
    gas_score = (0.75 / (gas_upper_limit - gas_lower_limit) * gas_reference - (gas_lower_limit * (0.75 / (gas_upper_limit - gas_lower_limit)))) * 100;

    float air_quality_score = hum_score + gas_score;

    float IAQ_Index = (100 - air_quality_score) * 5;
    IAQ_Level = get_iaq_level(IAQ_Index);

    Serial.printf("Air Quality Index = %f \nAir Quality Score = %f \nIAQ Level = %d \n", IAQ_Index, air_quality_score, IAQ_Level);
    Serial.println("------------------------------------------------");

	MyData data = MyData(
		Temperature,
		Humidity,
		lux,
        pmat10,
		pmat25,
		pmat100,
		IAQ_Index,
		IAQ_Level
	);

	send_db(data);
    send_thingspeak(data);

    delay(10000);
}

void connect_wifi() {
	Serial.print("Connecting to Wi-Fi SSID: ");

    Serial.println(ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
	while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(50);
    }

    Serial.println();

    Serial.println("Connection completed. ");
}

void i2c_init() {
	Wire.begin(); // i2c prepare
	Serial.println("I2C connect successful! ");
}

void bme_init() {
	Serial.println(F("BME680 testing... "));

	while (!bme.begin()) {
        Serial.println("Could not find a valid BME680 sensor, check wiring!");
        delay(1);
    }
	
	Serial.println("Found a BME680 sensor");

    bme.setTemperatureOversampling(BME680_OS_2X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_2X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150);
}

void bh1750_init() {
	Serial.println(F("BH1750 testing... "));

	while (!lightMeter.begin()) {
        Serial.println("Could not find a valid BH1750 sensor, check wiring!");
		delay(1);
    } 
	
	Serial.println("Found a BH1750 sensor");
}

void pms5003t_spec() {
    int count = 0;
    unsigned char c;
    unsigned char high = '\0';

    while (pmsSerial.available()) {
        c = pmsSerial.read();
        
        if ((count == 0 && c != 0x42) || (count == 1 && c != 0x4d)) {
            break;
        }

        if (count > 27) {
            break;
        }

        if (count == 10 || count == 12 || count == 14 || count == 24 || count == 26) {
            high = c;
        } else if (count == 11) {
            pmat10 = 256 * high + c;
        } else if (count == 13) {
            pmat25 = 256 * high + c;
        } else if (count == 15) {
            pmat100 = 256 * high + c;
        }
        count++;
    }
    while (pmsSerial.available())
        pmsSerial.read();
}

void get_gas_reference() {
    Serial.println("Getting a new gas reference value... ");
    int readings = 10;
    for (int i = 0; i <= readings; i++) {
        gas_reference += bme.readGas();
    }
    gas_reference = gas_reference / readings;
}

unsigned int get_iaq_level(float iaq_index) {
	if (iaq_index > 300) return 6;
    else if (iaq_index > 200) return 5;
    else if (iaq_index > 175) return 4;
    else if (iaq_index > 150) return 3;
    else if (iaq_index > 50) return 2;
    else return 1;
}

void send_db(MyData data) {
	if (client.connect(db_server, 80)) {
        String postStr = "{";
        postStr += "\"temperature\":";
        postStr += String(data.temperature);
        postStr += ",";
        postStr += "\"humidity\":";
        postStr += String(data.humidity);
        postStr += ",";
        postStr += "\"lux\":";
        postStr += String(data.lux);
        postStr += ",";
        postStr += "\"pm10\":";
        postStr += String(data.pm10);
        postStr += ",";
        postStr += "\"pm25\":";
        postStr += String(data.pm25);
        postStr += ",";
        postStr += "\"pm100\":";
        postStr += String(data.pm100);
        postStr += ",";
        postStr += "\"iaqIndex\":";
        postStr += String(data.iaq_index);
        postStr += ",";
        postStr += "\"iaqLevel\":";
        postStr += String(data.iaq_level);
        postStr += "}";

        client.print("POST /api/super-big-dick HTTP/1.1\r\n");
        client.printf("Host: %s\r\n", db_server);
        client.print("Connection: close\r\n");
        client.print("Content-Type: application/json\r\n");
        client.printf("Content-Length: %d\r\n", postStr.length());
        client.print("\r\n");
        client.print(postStr); 
        Serial.println(postStr);
    }
}

void send_thingspeak(MyData data) {
	if (client.connect(thingspeak_server, 80)) {
        String postStr = thingspeak_server_key;
        postStr += "&field1=";
        postStr += String(data.temperature);
        postStr += "&field2=";
        postStr += String(data.humidity);
        postStr += "&field3=";
        postStr += String(data.lux);
        postStr += "&field4=";
        postStr += String(data.pm10);
        postStr += "&field5=";
        postStr += String(data.pm25);
        postStr += "&field6=";
        postStr += String(data.pm100);
        postStr += "&field7=";
        postStr += String(data.iaq_index);
        postStr += "&field8=";
        postStr += String(data.iaq_level);
        postStr += "\r\n\r\n";

        client.println("POST /update HTTP/1.1");
        client.printf("Host: %s\r\n", thingspeak_server);
        client.println("Connection: close");
        client.printf("X-THINGSPEAKAPIKEY: %s\r\n", thingspeak_server_key);
        client.println("Content-Type: application/x-www-form-urlencoded");
        client.printf("Content-Length: %d\r\n", postStr.length());
        client.print("\r\n");
        client.print(postStr); 
    }
}
