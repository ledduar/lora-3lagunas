
#include <Arduino.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <RTClib.h>
#include <stdio.h>
#include <TimeLib.h>
#include <RHSoftwareSPI.h>

// LoRa parameters
long freq = 433E6;
long bw = 125E3;
int int_sf;
int int_cr;
String message = "mensaje ";
int counter = 0;
int repeticiones = 1;

int sf_array[] = {7, 8, 9, 10, 11, 12};
int size_sf_array = sizeof(sf_array) / sizeof(sf_array[0]);

int cr_array[] = {5, 6, 7, 8};
int size_cr_array = sizeof(cr_array) / sizeof(cr_array[0]);

//#define filas (2)
#define filas (24)
#define columnas (2)

int final_array[filas][columnas];
int contador_array = 0;
int contador_parametros = 0;

/* Marcas de tiempo */
int minuto_actual;
int minuto_envio;
int segundo_envio;

/* Pines Chip LoRa */
#define LORA_INT PIN_PD3
#define LORA_RST PIN_PC7
#define LORA_CS PIN_PC4

/* Pines SD card */
#define SD_CS PIN_PC3

/* LED_VERDE */
#define LED_VERDE PIN_PB0

/* LED_ROJO */
#define LED_ROJO PIN_PB1

// Time Wireshark
tmElements_t timeWS;

// packet counter
int16_t rxCount = 0;

// RTC
RTC_DS3231 rtc;

// Json Wireshark parameters
struct Config
{
  String payload;
  time_t sec;
  int rssi;
  float snr;
  long freq;
  int sf;
  long bw;
  int cr;
};

char charname[15];
const char *logname = "/log.txt";
Config config; // <- global configuration object

// Reset Arduino Programmatically
void(* resetFunc) (void) = 0;

// Saves the configuration to a file
void saveConfiguration(const char*filename, File &logfile, const Config &config)
{
  // Open file for writing
  File file = SD.open(filename, O_WRITE | O_CREAT | O_APPEND);
  if (!file)
  {
    Serial.println(" X Error al crear archivo en SD");
    logfile.println(" X Error al crear archivo en SD");
    return;
  }
  else
  {
    Serial.println(" - Archivo creado en SD");
    logfile.println(" - Archivo creado en SD");
  }

  // Document JSON
  StaticJsonDocument<1024> doc;

  // Set the values in the document
  doc["payload"] = config.payload;
  doc["sec"] = config.sec;
  doc["freq"] = config.freq;
  doc["sf"] = config.sf;
  doc["bw"] = config.bw;
  doc["cr"] = config.cr;
  rxCount++;

  Serial.print(" - Paquete almacenado Nro: ");
  Serial.println(rxCount);
  logfile.print(" - Paquete almacenado Nro: ");
  logfile.println(rxCount);

  // Serialize JSON to file
  if (serializeJsonPretty(doc, file))
  {
    Serial.println(" - JSON escrito en SD");
    logfile.println(" - JSON escrito en SD");
  }
  else
  {
    Serial.println(" X Error al escribir el JSON en SD");
    logfile.println(" X Error al escribir el JSON en SD");
  }

  // Close the file
  file.print(",");
  file.close();

  // LED_VERDE
  digitalWrite(LED_VERDE, HIGH);
  delay(500);
  digitalWrite(LED_VERDE, LOW);
}

// Prints the content of a file to the Serial
void printFile(const char *filename)
{
  // Open file for reading
  File file = SD.open(filename);
  if (!file)
  {
    Serial.println("Failed to read file");
    return;
  }

  // Extract each characters by one by one
  while (file.available())
  {
    Serial.print((char)file.read());
  }
  Serial.println();

  // Close the file
  file.close();
}

void setup()
{

  // Initialize serial port
  Serial.begin(115200);
  while (!Serial)
    ;

  // LED_VERDE
  pinMode(LED_VERDE, OUTPUT);
  // LED_ROJO
  pinMode(LED_ROJO, OUTPUT);

  // Set pins
  LoRa.setPins(LORA_CS, LORA_RST, LORA_INT);

  // LoRa radio
  if (!LoRa.begin(freq))
  {
    Serial.println("LoRa begin failed. Check your connections.");
    digitalWrite(LED_VERDE, HIGH);
    Serial.println("Reiniciando en 2 segundos");
    delay(2000);
    digitalWrite(LED_VERDE, LOW);
    resetFunc();
  } else
  {
    Serial.println("LoRa init OK!");
    digitalWrite(LED_VERDE, HIGH);
    delay(50);
    digitalWrite(LED_VERDE, LOW);
    delay(50);
    digitalWrite(LED_VERDE, HIGH);
    delay(50);
    digitalWrite(LED_VERDE, LOW);
  }

  LoRa.setSyncWord(0x39);
  LoRa.setSignalBandwidth(bw);

  // SD Card
  if (!SD.begin(SD_CS))
  {
    Serial.println("SD Card: mounting failed.");
    digitalWrite(LED_ROJO, HIGH);
    Serial.println("Reiniciando en 2 segundos");
    delay(2000);
    digitalWrite(LED_ROJO, LOW);
    resetFunc();
  }
  else
  {
    Serial.println("SD Card: mounted.");
    digitalWrite(LED_ROJO, HIGH);
    delay(50);
    digitalWrite(LED_ROJO, LOW);
    delay(50);
    digitalWrite(LED_ROJO, HIGH);
    delay(50);
    digitalWrite(LED_ROJO, LOW);
  }

  // RTC begin
  if (!rtc.begin()) {
    Serial.println("X Modulo RTC no encontrado");
    while (true);
  }

  // Vector de combinaciones
  for (int i = 0; i < size_cr_array; i++)
  {
    for (int j = 0; j < size_sf_array; j++)
    {
      final_array[contador_array][0] = sf_array[j];
      final_array[contador_array][1] = cr_array[i];
      contador_array++;
    }
  }
}

void loop()
{

  // Open file for writing
  File logfile = SD.open(logname, O_WRITE | O_CREAT | O_APPEND);
  if (!logfile)
  {
    Serial.println("X Error al crear log en SD");
    digitalWrite(LED_ROJO, HIGH);
    return;
  } else {
    digitalWrite(LED_ROJO, LOW);
  }

  // time RTC
  DateTime fecha = rtc.now();
  Serial.print(">>>>> Tiempo actual: ");
  Serial.print(fecha.hour());
  Serial.print(":");
  Serial.print(fecha.minute());
  Serial.print(":");
  Serial.println(fecha.second());

  minuto_actual = fecha.minute();

  if (minuto_actual != minuto_envio)
  {
    Serial.println("############# Cambio el minuto #############");

    const char *file_m = "/m";
    const char *file_sf = "s";
    const char *file_cr = "c";
    const char *extension = ".txt";

    Serial.print("SF: ");
    int_sf = final_array[contador_parametros][0];
    LoRa.setSpreadingFactor(int_sf);
    char char_sf[10];
    itoa (int_sf, char_sf, 10);
    Serial.println(int_sf);

    Serial.print("CR: ");
    int_cr = final_array[contador_parametros][1];
    LoRa.setCodingRate4(int_cr);
    char char_cr[10];
    Serial.println(int_cr);
    itoa (int_cr, char_cr, 10);

    Serial.print("Muestra: ");
    char char_rep[10];
    Serial.println(repeticiones);
    itoa (repeticiones, char_rep, 10);

    Serial.print("Minuto actual: ");
    Serial.println(minuto_actual);
    Serial.print("Minuto envio: ");
    Serial.println(minuto_envio);

    strcpy(charname, file_m);
    strcat(charname, char_rep);
    strcat(charname, file_sf);
    strcat(charname, char_sf);
    strcat(charname, file_cr);
    strcat(charname, char_cr);
    strcat(charname, extension);
    Serial.println(charname);
    
    Serial.print("contador_parametros: ");
    Serial.println(contador_parametros);

    Serial.println("############# Cambio el minuto #############");
    contador_parametros++;
  }

  if (contador_parametros == filas + 1) {
    contador_parametros = 0;
    rxCount = 0;
    counter = 0;

    if (repeticiones == 3) {
      logfile.close();
      Serial.println("*********************************************************");
      Serial.println("****************** FIN DEL EXPERIMENTO ******************");
      Serial.println("****************** FIN DEL EXPERIMENTO ******************");
      Serial.println("****************** FIN DEL EXPERIMENTO ******************");
      Serial.println("*********************************************************");
      while (true) {
        digitalWrite(LED_VERDE, HIGH);
        digitalWrite(LED_ROJO, HIGH);
        delay(1000);
        digitalWrite(LED_VERDE, LOW);
        digitalWrite(LED_ROJO, LOW);
        delay(1000);
      };
    };

    repeticiones++;
    Serial.println("=============== Analisis parcial completo ===============");
    Serial.println("=============== Analisis parcial completo ===============");
    Serial.println("=============== Analisis parcial completo ===============");
    return;
  }

  Serial.print(">>>>> Combinacion: ");
  Serial.println(charname);
  Serial.println("_____ENVIANDO NUEVO PAQUETE_____");
  counter++;
  Serial.print("Payload: ");
  Serial.print(message);
  Serial.println(counter);

  logfile.println("_____ENVIANDO NUEVO PAQUETE_____");
  logfile.print("Payload: ");
  logfile.print(message);
  logfile.println(counter);

  // send packet
  LoRa.beginPacket();
  minuto_envio = fecha.minute();
  segundo_envio = fecha.second();
  LoRa.print(message);
  LoRa.print(counter);
  LoRa.endPacket();

  timeWS.Hour = fecha.hour() + 4;
  timeWS.Minute = minuto_envio;
  timeWS.Second = segundo_envio;
  timeWS.Day = fecha.day();
  timeWS.Month = fecha.month();
  timeWS.Year = fecha.year() - 1970;
  time_t t_sec = makeTime(timeWS);

  // Copy values from the JsonDocument to the Config
  config.payload = message + "" + String(counter);
  config.sec = t_sec;
  config.freq = freq;
  config.sf = int_sf;
  config.bw = bw;
  config.cr = int_cr;

  // Tiempo en log
  logfile.print("Tiempo: ");
  logfile.println(t_sec);

  // Create configuration file
  Serial.println("Guardando configuracion...");
  const char* filename = charname;
  saveConfiguration(filename, logfile, config);

  // Save
  logfile.close();

  // Delay
  delay(1000);
  
}
