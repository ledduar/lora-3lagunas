
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
boolean primer_paquete_recibido = false;
int repeticiones = 1;

/* Arreglos de parametros */
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

/* Pines Chip LoRa */
#define LORA_INT 26
#define LORA_RST 14
#define LORA_CS 18
#define LORA_SCK 5
#define LORA_MOSI 27
#define LORA_MISO 19

/* Pines SD card */
#define SD_CS 2
#define SD_MOSI 23
#define SD_SCK 17
#define SD_MISO 13

// SPI bus virtual
SPIClass sd_spi(HSPI);

/* LED */
#define LED 25
#define LED_AZUL 12

// Tiempo Wireshark
tmElements_t timeWS;

// Contador de paquetes
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

// Saves the configuration to a file
void saveConfiguration(const char *filename, File &logfile, const Config &config)
{
  // Open file for writing
  File file = SD.open(filename, FILE_APPEND);
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
  doc["rssi"] = config.rssi;
  doc["snr"] = config.snr;
  doc["freq"] = config.freq;
  doc["sf"] = config.sf;
  doc["bw"] = config.bw;
  doc["cr"] = config.cr;
  rxCount++;

  Serial.print(" - Paquete recibido Nro: ");
  Serial.println(rxCount);
  logfile.print(" - Paquete recibido Nro: ");
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

  // LED
  digitalWrite(LED, HIGH);
  delay(500);
  digitalWrite(LED, LOW);
}

void setup()
{

  // Initialize serial port
  Serial.begin(115200);
  while (!Serial)
    ;

  // LED
  pinMode(LED, OUTPUT);
  pinMode(LED_AZUL, OUTPUT);

  // Set pins
  LoRa.setPins(LORA_CS, LORA_RST, LORA_INT);

  // LoRa radio
  if (!LoRa.begin(freq))
  {
    Serial.println("LoRa init failed. Check your connections.");
    digitalWrite(LED, HIGH);
    Serial.println("Reiniciando en 2 segundos");
    delay(2000);
    ESP.restart();
  }
  else
  {
    Serial.println("LoRa init OK!");
    digitalWrite(LED, HIGH);
    delay(50);
    digitalWrite(LED, LOW);
    delay(50);
    digitalWrite(LED, HIGH);
    delay(50);
    digitalWrite(LED, LOW);
  }

  LoRa.setSyncWord(0x39);
  LoRa.setSignalBandwidth(bw);

  // SD Card
  sd_spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, sd_spi))
  {
    Serial.println("SD Card: mounting failed.");
    digitalWrite(LED_AZUL, HIGH);
    Serial.println("Reiniciando en 2 segundos");
    delay(2000);
    ESP.restart();
  }
  else
  {
    Serial.println("SD Card: mounted.");
    digitalWrite(LED_AZUL, HIGH);
    delay(50);
    digitalWrite(LED_AZUL, LOW);
    delay(50);
    digitalWrite(LED_AZUL, HIGH);
    delay(50);
    digitalWrite(LED_AZUL, LOW);
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
  File logfile = SD.open(logname, FILE_APPEND);
  if (!logfile)
  {
    Serial.println("X Error al crear log en SD");
    digitalWrite(LED_AZUL, HIGH);
    return;
  } else {
    digitalWrite(LED_AZUL, LOW);
  }

  // Tiempo RTC
  DateTime fecha = rtc.now();

  minuto_actual = fecha.minute();

  if (!primer_paquete_recibido)
  {
    Serial.println("Esperando paquetes....");
    Serial.println("Esperando paquetes....");
    Serial.println("Esperando paquetes....");
    Serial.println("Esperando paquetes....");
    Serial.println("Esperando paquetes....");
    minuto_envio = minuto_actual + 5;
  }

  if (minuto_actual != minuto_envio)
  {
    
    if (primer_paquete_recibido)
    {
      contador_parametros++;
    }
    
    Serial.println("############# Cambio el minuto #############");

    const char *file_m = "/m";
    const char *file_sf = "s";
    const char *file_cr = "c";
    const char *extension = ".txt";

    Serial.print("SF: ");
    int_sf = final_array[contador_parametros][0];
    LoRa.setSpreadingFactor(int_sf);
    char char_sf[10];
    itoa(int_sf, char_sf, 10);
    Serial.println(int_sf);

    Serial.print("CR: ");
    int_cr = final_array[contador_parametros][1];
    LoRa.setCodingRate4(int_cr);
    char char_cr[10];
    Serial.println(int_cr);
    itoa(int_cr, char_cr, 10);

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

    minuto_envio = minuto_actual;
    
    Serial.println("############# Cambio el minuto #############");
  }

  if (contador_parametros == filas) {
    contador_parametros = 0;
    rxCount = 0;
    primer_paquete_recibido = false;
    minuto_envio = minuto_actual + 5; //diferente

    if (repeticiones == 3) {
      logfile.close();
      Serial.println("*********************************************************");
      Serial.println("****************** FIN DEL EXPERIMENTO ******************");
      Serial.println("****************** FIN DEL EXPERIMENTO ******************");
      Serial.println("****************** FIN DEL EXPERIMENTO ******************");
      Serial.println("*********************************************************");
      while (true) {
        digitalWrite(LED, HIGH);
        digitalWrite(LED_AZUL, HIGH);
        delay(1000);
        digitalWrite(LED, LOW);
        digitalWrite(LED_AZUL, LOW);
        delay(1000);
      };
    };

    repeticiones++;
    Serial.println("=============== Analisis parcial completo ===============");
    Serial.println("=============== Analisis parcial completo ===============");
    Serial.println("=============== Analisis parcial completo ===============");
    return;
  }

  // try to parse packet
  int packetSize = LoRa.parsePacket();

  if (packetSize)
  {

    // Se recibio un paquete
    primer_paquete_recibido = true;

    minuto_envio = fecha.minute();
    
    timeWS.Hour = fecha.hour() + 4;
    timeWS.Minute = minuto_envio;
    timeWS.Second = fecha.second();
    timeWS.Day = fecha.day();
    timeWS.Month = fecha.month();
    timeWS.Year = fecha.year() - 1970;
    time_t t_sec = makeTime(timeWS);

    // received a packet
    Serial.print(">>>>> Tiempo actual: ");
    Serial.print(fecha.hour());
    Serial.print(":");
    Serial.print(fecha.minute());
    Serial.print(":");
    Serial.println(fecha.second());
    Serial.print(">>>>> Combinacion: ");
    Serial.println(charname);
    Serial.println("_____NUEVO PAQUETE RECIBIDO_____");
    Serial.print("Payload: ");
    String message = "";
    // read packet
    while (LoRa.available())
    {
      message += (char)LoRa.read();
    }
    Serial.println(message);

    logfile.println("_____NUEVO PAQUETE RECIBIDO_____");
    logfile.print("Payload: ");
    logfile.println(message);

    Serial.print("RSSI: ");
    Serial.println(LoRa.packetRssi());

    // Copy values from the JsonDocument to the Config
    config.payload = message;
    config.sec = t_sec;
    config.rssi = LoRa.packetRssi();
    config.snr = LoRa.packetSnr();
    config.freq = freq;
    config.sf = int_sf;
    config.bw = bw;
    config.cr = int_cr;

    // Tiempo en log
    logfile.print("Tiempo: ");
    logfile.println(t_sec);

    // Create configuration file
    Serial.println("Guardando configuracion...");
    const char *filename = charname;
    saveConfiguration(filename, logfile, config);
    
  }

  // Save
  logfile.close();
}
