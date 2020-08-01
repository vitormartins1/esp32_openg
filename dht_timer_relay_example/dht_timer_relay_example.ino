/*
Este exemplo requer:
ESP32 WIFI
DHT22 ou DHT11
RELE de 2 canais no minimo
Blynk App
*/


#define BLYNK_PRINT Serial    // Comente isto para desabilitar prints e economizar espaco

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <WidgetRTC.h>

// BLYNK AUTHENTICATION
char auth[] = "sua autenticacao";
char ssid[] = "seu wifi";
char pass[] = "senha do seu wifi";

// DHT TEMPERATURE AND HUMIDIDTY
#define DHTPIN 15     // definicao do pino do sensor de umidade e temperatura dht
#define DHTTYPE DHT22 // mudar para DHT11 caso esteja usando outra versao do sensor
DHT dht(DHTPIN, DHTTYPE);

// TIMER, ALARMS AND RTC
BlynkTimer timer;     // usamos o timer do blynk
WidgetRTC rtc;        // o real time clock do blynk nos da o horario atualizado via wifi

// RELAY
const int LEDS = 16;     //110v
const int COOLERS = 18;  //12v

// evento chamado a cada 5 segundos para atualizar o display do relogio
void alarmTimerEvent() {
  digitalClockDisplay();  // debug do horario
  Alarm.delay(0);   // necesario para sincronizacao do alarme
  
  String currentTime = String(hour()) + ":" + minute() + ":" + second();
  // atualizo um label no blynk para saber se o horario esta correto
  // necessario somente para testes
  Blynk.virtualWrite(V0, currentTime); 
}

// isso e chamado quando o esp32 se conecta ao blynk server
BLYNK_CONNECTED() {
  rtc.begin();        // iniciamos o real time clock do blynk
  Alarm.delay(1000);  // necessario para os alarmes ficarem sincronizados
}

// isto e chamado pelo blynk quando um virtual pin e alterado no app
// nesse caso o v2 virtual pin 2 esta configurado no widget de time input
// com start-stop time, para ter um alarme de inicio e fim do fotoperiodo
BLYNK_WRITE(V2) {
  TimeInputParam t(param);

  // processa o horario de acender as luzes
  if (t.hasStartTime()) {
    Alarm.free(0);
    Alarm.alarmRepeat(t.getStartHour(),
                      t.getStartMinute(),
                      t.getStartSecond(), 
                      GrowInitialAlarm);
    
    Serial.println(String("Start: ") + t.getStartHour() + ":" + t.getStartMinute() + ":" + t.getStartSecond());
  }

  // processa o horario de desligar as luzes
  if (t.hasStopTime()) {    
    Alarm.free(1);
    Alarm.alarmRepeat(t.getStopHour(),
                      t.getStopMinute(),
                      t.getStopSecond(), 
                      GrowFinalAlarm);
    
    Serial.println(String("Stop: ") + t.getStopHour() + ":" + t.getStopMinute() + ":" + t.getStopSecond());
  }
}

// esse evento e chamado a cada segundo para se ter um dado robusto 
// de umidade e temperatura a mostrar no graph do blynk. o graph do blynk
// usa automaticamente os valores dos virtual pins para gerar os graficos
// nao havendo necessidade de codificar 
void dhtTimerEvent() {
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // ou dht.readTemperature(true) para Fahrenheit

  // verificamos caso o sensor tenha gerado um dado descartavel
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT22 sensor!");
    return;
  }

  // atualizamos a umidade
  Blynk.virtualWrite(V5, h);
  // e atualizamos a temperatura
  Blynk.virtualWrite(V6, t);
}

void setup() {
Serial.begin(9600);

  Blynk.begin(auth, ssid, pass);

  setSyncInterval(10 * 60);   // intervalo de sincronizacao do horario a cada 10 minutos(em segundos)
  setTime(hour(),minute(),second(),month(),day(),year());
  Serial.print("setup RTC Updated Time ");
  timer.setInterval(1000L, dhtTimerEvent);    // Chamado a cada segundo
  timer.setInterval(5000L, alarmTimerEvent);  // display digital atualiza a cada 5 segundos
  digitalClockDisplay();

  // inicializa o sensor de umidade e temperatura dht
  dht.begin();          

  // inicializamos os alarmes e reles
  setupAlarms();
  setupRelays();
}

// usando o blynk e importante nao usar o loop
// caso precise de um loop, use os timer event
void loop() {
  Blynk.run();
  timer.run();
}

// aciona os reles para acender as luzes e ligar os ventiladores
void GrowInitialAlarm() {
  digitalWrite(LEDS, HIGH);
  digitalWrite(COOLERS, HIGH);
}

// aciona os reles para desligar as luzes e desligar os ventiladores
void GrowFinalAlarm() {
  digitalWrite(LEDS, LOW);
  digitalWrite(COOLERS, LOW);
}

// configuracao inicial dos alarmes
void setupAlarms() {
  Alarm.alarmRepeat(0, 0, 0, GrowInitialAlarm);
  Alarm.disable(0);
  
  Alarm.alarmRepeat(0, 0, 0, GrowFinalAlarm);
  Alarm.disable(1);
}

// configuracao inicial dos reles
void setupRelays() {
  pinMode(LEDS, OUTPUT);
  pinMode(COOLERS, OUTPUT);
  
  digitalWrite(LEDS, LOW);
  digitalWrite(COOLERS, LOW);
}

// debug do relogio
void digitalClockDisplay() {
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.println(); 
}

// metodo que auxilia a printar o horario 
void printDigits(int digits) {
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}
