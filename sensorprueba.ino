#include <HTTPClient.h>
#include "WiFi.h"

const float Vcc = 3.3;
int estadoInputPin[2] = {27,17};
int ledInputPin[2] = {26,16};
int sensorInputPin[2] = {33, 34};
const float K = 2.5;
float A = 2.229674985e-03, B = 1.211871252e-04, C = 8.713435086e-07;
float valuesSensors[2] = {0,0};

const int tiempoEnvioCompletado = 5000;
const int tiempoSensorCompletado = 1000;

/*CREDENTIALS*/
const char* ssid = "ECOSATWIFI";
const char* password = "ECOSAT2010";
const char* ipServer = "ecosat.com.mx";
const char* puertoServer = "80";

int tiempoEnvio = 0;
int tiempoSensor = 0;
int tiempoDelay = 250;



int estadoAnterior[] = {true, true};

/**
   Funcion para inicializar wifi.
*/
void inicializarWifi() {
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

void inicializarSensores() {
  pinMode(estadoInputPin[0], INPUT);
  pinMode(estadoInputPin[1], INPUT);
  pinMode(ledInputPin[0], OUTPUT);
  pinMode(ledInputPin[1], OUTPUT);
}


void setup() {
  Serial.begin(115200);

  tiempoEnvio = 0;
  tiempoSensor = 0;

  inicializarWifi();
  inicializarSensores();
}

void sensortermistor()
{
  for(int i = 0; i < 2; i++){
    float raw = analogRead(sensorInputPin[i]);
    float V = raw / 4095 * Vcc;

    float R = (R1 * V) / (Vcc - V);

    float logR = log(R);
    float R_th = 1.0 / (A + B * logR + C * logR * logR * logR );

    float kelvin = R_th - V*V/(K*R)*1000;
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
  } else {
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

void sensorEstado(int sE, int l, String msg, int id) {
  //    TRUE == HIGH
  //    FALSE == LOW
  bool estadoActual=0;

  if (digitalRead(sE) == HIGH) {
    estadoActual = 1;
  } else {
    estadoActual = 0;
  }

  if (estadoActual != estadoAnterior[id - 1]) {
    if (digitalRead(sE) == HIGH ) { //Cierra puerta
      digitalWrite(l, LOW); //Enciende ledInputPin[
]      Serial.println("APAGADO " + msg);
      estadoAnterior[id - 1] = 1;
      SendRoomInfo(ipServer,puertoServer,id,"0","insert-sensor");
    } else { //Abre Puerta
      digitalWrite(l, HIGH); //Apaga ledInputPin[
]      Serial.println("ENCENDIDO " + msg);
      estadoAnterior[id - 1] = 0;
      SendRoomInfo(ipServer,puertoServer,id,"1","insert-sensor");
    }
  }
}

void loop() {
  // Wait a few seconds between measurements.
  sensorEstado(estadoInputPin[0], ledInputPin[0], " Principal", 1);
  sensorEstado(estadoInputPin[1], ledInputPin[1], " Secundario", 2);
  if (WiFi.status() == WL_CONNECTED) {
    if (tiempoSensor == tiempoSensorCompletado) {
      tiempoSensor = 0;
      sensortermistor();
    } else if (tiempoEnvio == tiempoEnvioCompletado) {
      //      Serial.println("Aun no se enviara la informacion por el tiempoTranscurridoSensor: " + String(tiempoSensor, DEC) + " del tiempo sensor completado: " + String(tiempoSensorCompletado, DEC));
    }
  } else {
    if (tiempoSensor == tiempoSensorCompletado) {
      tiempoSensor = 0;
      sensortermistor();
    } else {
      //      Serial.println("Aun no se enviara la informacion por el tiempoTranscurridoSensor: " + String(tiempoSensor, DEC) + " del tiempo sensor completado: " + String(tiempoSensorCompletado, DEC));
      //      Serial.println("Sin conexion a internet intentaremos hacer reconexion");
      sensortermistor();
    }
  }
  if (tiempoEnvio == tiempoEnvioCompletado) {
    tiempoEnvio = 0;
    SendRoomInfo(ipServer, puertoServer, 1, String(valuesSensors[0], 3), "update-temperature");
    SendRoomInfo(ipServer, puertoServer, 2, String(valuesSensors[1], 3), "update-temperature");
  } else {
    //    Serial.println("Aun no se enviara la informacion por el tiempoTranscurridoEnvio: " + String(tiempoEnvio, DEC) + " del tiempo a enviar: " + String(tiempoEnvioCompletado, DEC));
  }
  tiempoSensor = tiempoSensor + tiempoDelay;
  tiempoEnvio = tiempoEnvio + tiempoDelay;
  delay(tiempoDelay);
}
