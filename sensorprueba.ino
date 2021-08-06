#include <HTTPClient.h>
#include "WiFi.h"

const int sEst0 = 27;
const int sEst1 = 17;
const int led0 = 26;
const int led1 = 16;
const int tiempoEnvioCompletado = 5000;
const int tiempoSensorCompletado = 1000;
const char* ssid = "ECOSATWIFI";
const char* password = "ECOSAT2010";
const char* ipServer = "ecosat.com.mx";
const char* puertoServer = "80";
int tiempoEnvio = 0;
int tiempoSensor = 0;
int tiempoDelay = 250;
int Vo;
float R1 = 1000;
float logR2, R2, TEMPERATURASENSOR;
float c1 = 2.229674985e-03, c2 = 1.211871252e-04, c3 = 8.713435086e-07;
int Voa;
float logR2a, R2a, TEMPERATURASENSORa;
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
  pinMode(sEst0, INPUT);
  pinMode(sEst1, INPUT);
  pinMode(led0, OUTPUT);
  pinMode(led1, OUTPUT);
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
  Vo = analogRead(33);
  R2 = R1 * (1023.0 / ((float)Vo - 1.0));
  logR2 = log(R2);
  TEMPERATURASENSOR = (1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2));
  TEMPERATURASENSOR = TEMPERATURASENSOR - 273.15;
  Serial.print("TemperaturaSENSOR: ");
  Serial.print(TEMPERATURASENSOR);
  Serial.println(" C");

  Voa = analogRead(34);
  R2a = R1 * (1023.0 / ((float)Voa - 1.0));
  logR2a = log(R2a);
  TEMPERATURASENSORa = (1.0 / (c1 + c2 * logR2a + c3 * logR2a * logR2a * logR2a));
  TEMPERATURASENSORa = TEMPERATURASENSORa - 273.15;
  Serial.print("TemperaturaSENSOR 2: ");
  Serial.print(TEMPERATURASENSORa);
  Serial.println(" C");
}

void SendData(String ip, String puerto, String method, int base , int numero, int id, int estado, float TEMPERATURA, String fecha)
{
  HTTPClient http;
  String dataline;

  //dataline = "http://"+ip+":"+puerto+"/test/index.php?method="+method+"&numero="+String(numero, DEC)+"&id="+String(id, DEC)+"&estado="+String(estado, DEC)+"&valor="+String(TEMPERATURA, DEC)+"&fecha="+fecha;
  dataline = "http://" + ip + ":" + puerto + "/test/index.php?method=" + method + "&numero=" + String(numero, DEC) + "&id=" + String(id, DEC) + "&estado=" + String(estado, DEC) + "&valor=" + String(TEMPERATURA, DEC) + "&fecha=" + fecha;

  //http://192.168.10.198/test_temper/vistas/test.php?method=estados&numero=1&id=0&estado=1&valor=20.25
  //http://192.168.10.195/test/insert-sensor.php?id=1&data=0

  Serial.println(dataline);
  
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
      Serial.println(payload);
    }
    else
    { // Request could not be made
      Serial.printf("HTTP request failed. Error: %s\r\n", http.errorToString(httpCode).c_str());
    }
  }
  http.end();
}

void SendRoomInfo(String ip, String puerto, int id, String data, String namefile)
{
  HTTPClient http;
  String dataline;
  dataline = "https://" + ip + "/aplicaciones/temperatura/" + namefile + ".php?&id=" + String(id, DEC) + "&data=" + data;

  //http://192.168.10.195/test/insert-sensor.php?id=1&data=0

//  Serial.println(dataline);

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
      Serial.printf("HTTP request failed. Error: %s\r\n", http.errorToString(httpCode).c_str());
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
      digitalWrite(l, LOW); //Enciende LED
      Serial.println("APAGADO " + msg);
      estadoAnterior[id - 1] = 1;
      SendRoomInfo(ipServer,puertoServer,id,"0","insert-sensor");
    } else { //Abre Puerta
      digitalWrite(l, HIGH); //Apaga LED
      Serial.println("ENCENDIDO " + msg);
      estadoAnterior[id - 1] = 0;
      SendRoomInfo(ipServer,puertoServer,id,"1","insert-sensor");
    }
  }
}

void loop() {
  // Wait a few seconds between measurements.
  sensorEstado(sEst0, led0, " Principal", 1);
  sensorEstado(sEst1, led1, " Secundario", 2);
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
    SendRoomInfo(ipServer, puertoServer, 1, String(TEMPERATURASENSOR, 3), "update-temperature");
    SendRoomInfo(ipServer, puertoServer, 2, String(TEMPERATURASENSORa, 3), "update-temperature");
  } else {
    //    Serial.println("Aun no se enviara la informacion por el tiempoTranscurridoEnvio: " + String(tiempoEnvio, DEC) + " del tiempo a enviar: " + String(tiempoEnvioCompletado, DEC));
  }
  tiempoSensor = tiempoSensor + tiempoDelay;
  tiempoEnvio = tiempoEnvio + tiempoDelay;
  delay(tiempoDelay);
}
