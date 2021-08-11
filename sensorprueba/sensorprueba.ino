#include <HTTPClient.h>
#include "WiFi.h"

#define Rc 1000 //Resistance Value
#define Vcc 3.3 //Voltage Common
//Steinhart–Hart coefficients
float A = 2.229674985e-3;
float B = 1.211871252e-4;
float C = 8.713435086e-7;
const float K = 2.5; //Dissipation factor mW/C

int estadoInputPin[] = {27, 17};
int ledInputPin[] = {26, 16};
int sensorInputPin[] = {33, 34};
float valuesSensors[] = {0, 0};

int estadoAnterior[] = {1, 1};

const int tiempoEnvioCompletado = 5000;
const int tiempoSensorCompletado = 1000;

/*CREDENTIALS*/
const char *ssid = "Ecosat";
const char *password = "ECOSAT2021";
const char *ipServer = "ecosat.com.mx";
const char *puertoServer = "80";

int tiempoEnvio = 0;
int tiempoSensor = 0;
int tiempoDelay = 250;

/**
   Funcion para inicializar wifi.
*/
void inicializarWifi()
{
  /**
    Parametros de wifi
  */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Conectando a:\t");
  Serial.println(ssid);

  /**
     Esperamos a que haga conexion al wifi.
  */
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(200);
    Serial.print('.');
  }

  Serial.println();
  Serial.print("Conectado a:\t");
  Serial.println(WiFi.SSID());
  Serial.print("IP address;\t");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address;\t");
  Serial.println(WiFi.macAddress());
}

void inicializarSensores()
{
  pinMode(estadoInputPin[0], INPUT);
  pinMode(estadoInputPin[1], INPUT);
  pinMode(ledInputPin[0], OUTPUT);
  pinMode(ledInputPin[1], OUTPUT);
}

void setup()
{
  Serial.begin(115200);

  tiempoEnvio = 0;
  tiempoSensor = 0;

  inicializarWifi();
  inicializarSensores();
}

void sensortermistor()
{
  for (int i = 0; i < 2; i++)
  {
    float raw = analogRead(sensorInputPin[i]);
    float V = raw / 4095 * Vcc;

    float R = (Rc * V) / (Vcc - V);

    float logR = log(R);
    float R_th = 1.0 / (A + B * logR + C * logR * logR * logR);

    float kelvin = R_th - V * V / (K * R);
    valuesSensors[i] = kelvin - 273.15;

    Serial.print("Temperatura: ");
    Serial.print(valuesSensors[i]);
    Serial.println(" C");
  }
}

void SendRoomInfo(String ip, String puerto, int id, String data, String namefile)
{
  HTTPClient http;
  String dataline;
  dataline = "https://" + ip + "/aplicaciones/temperatura/" + namefile + ".php?&id=" + String(id, DEC) + "&data=" + data;

  //http://ecosat.com.mx/aplicaciones/temperatura/insert-sensor.php?id=1&data=0

  bool httpResult = http.begin(dataline);
  if (!httpResult)
  {
    Serial.println("Error when sending to server: ");
    Serial.println(dataline);
  }
  else
  {
    int httpCode = http.GET();
    if (httpCode > 0)
    { // Request has been made
      Serial.printf("HTTP status: %d Message: ", httpCode);
      String payload = http.getString();
      //      Serial.println(payload);
    }
    else
    { // Request could not be made
      Serial.printf("HTTP request failedInputPin[.] Error: %s\r\n", http.errorToString(httpCode).c_str());
    }
  }
  http.end();
}

void sensorEstado(String msg)
{
  //    TRUE == HIGH
  //    FALSE == LOW
  bool estadoActual = 0;
  for (int i = 0; i < 2; i++)
  {
    if (digitalRead(estadoInputPin[i]) == HIGH)
    {
      estadoActual = 1;
    }
    else
    {
      estadoActual = 0;
    }

    if (estadoActual != estadoAnterior[i])
    {
      if (digitalRead(estadoInputPin[i]) == HIGH)
      {                       //Cierra puerta
        digitalWrite(ledInputPin[i], LOW); //Enciende ledInputPin[i]
        Serial.println("APAGADO " + msg);
        estadoAnterior[i] = 1;
        SendRoomInfo(ipServer, puertoServer, id, "0", "insert-sensor");
      }
      else
      {                        //Abre Puerta
        digitalWrite(ledInputPin[i], HIGH); //Apaga ledInputPin[i]
        Serial.println("ENCENDIDO " + msg);
        estadoAnterior[i] = 0;
        SendRoomInfo(ipServer, puertoServer, id, "1", "insert-sensor");
      }
    }
  }
}

void loop()
{
  // Wait a few seconds between measurements.
  sensorEstado(" Principal");
  sensorEstado(" Secundario");
  if (WiFi.status() == WL_CONNECTED)
  {
    if (tiempoSensor == tiempoSensorCompletado)
    {
      tiempoSensor = 0;
      sensortermistor();
    }
    else if (tiempoEnvio == tiempoEnvioCompletado)
    {
      //      Serial.println("Aun no se enviara la informacion por el tiempoTranscurridoSensor: " + String(tiempoSensor, DEC) + " del tiempo sensor completado: " + String(tiempoSensorCompletado, DEC));
    }
  }
  else
  {
    if (tiempoSensor == tiempoSensorCompletado)
    {
      tiempoSensor = 0;
      sensortermistor();
    }
    else
    {
      //      Serial.println("Aun no se enviara la informacion por el tiempoTranscurridoSensor: " + String(tiempoSensor, DEC) + " del tiempo sensor completado: " + String(tiempoSensorCompletado, DEC));
      //      Serial.println("Sin conexion a internet intentaremos hacer reconexion");
      sensortermistor();
    }
  }
  if (tiempoEnvio == tiempoEnvioCompletado)
  {
    tiempoEnvio = 0;
    //TODO: FIX, SEND TEMPERATURE INFO
    SendRoomInfo(ipServer, puertoServer, 1, String(valuesSensors[0], 3), "update-temperature");
    SendRoomInfo(ipServer, puertoServer, 2, String(valuesSensors[1], 3), "update-temperature");
  }
  else
  {
    //    Serial.println("Aun no se enviara la informacion por el tiempoTranscurridoEnvio: " + String(tiempoEnvio, DEC) + " del tiempo a enviar: " + String(tiempoEnvioCompletado, DEC));
  }
  tiempoSensor = tiempoSensor + tiempoDelay;
  tiempoEnvio = tiempoEnvio + tiempoDelay;
  delay(tiempoDelay);
}
