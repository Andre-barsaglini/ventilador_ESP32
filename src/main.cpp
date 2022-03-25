#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "credentials.h"


//bits para atuar a entrada digital do inversor. Necessário utilizar o "run" para ligar e desligar o inversor, 
//mesmo usando a entrada analógica.
#define RUNADDR 17
#define BIT1ADDR 4
#define BIT2ADDR 2
#define BIT3ADDR 15
#define SPI_CS 22

//rede e socket. credenciais do wifi devem ser mantidas no arquivo credentials.h 
#define HOSTNAME "Ventilador" //wireless
#define PORTA 6969            //socket
#define PERIODO 1000          //periodo de reconexao e update em ms
IPAddress local_IP(192,168,1,169);  //wireless
IPAddress gateway(192, 168, 1, 1);  //wireless
IPAddress subnet(255, 255, 0, 0);   //wireless
WiFiServer sv(PORTA); //socket
WiFiClient cl;        //socket

// configurações e modos
bool printTestMode = true; // liga ou desliga todas as mensagens de teste via serial
bool closeAfterRec = false; // o host fecha o socket apos receber a mensagem

//tasks
TaskHandle_t taskTcp, taskCheckConn, taskLoop, taskRPM;
void taskTcpCode(void * parameter); // faz a comunicação via socket
void taskCheckConnCode(void * parameters); //checa periodicamente o wifi e verifica se tem atualização
void taskRPMCode(void * parameters); // controla o inversor de frequencia

//funcoes
void checkValue(); //avalia a mensagem recebida via tcp e 
void connectWiFi(); 
void bootSequence(); // dispara as tasks. 
void RPMAnalogico(); 
void RPMDigital();
void RPMStart();
void RPMStop();
void RPMCode();
void pot(); // controla o potenciometro digital
void printTest(); //função que printa via serial 

//modo de operação e status
int const coreTask = 0; //core onde rodarão as tasks nao relacionadas a comunicação (controle do pot/saidas digitais no caso)
char mensagemTcpIn[64] = ""; //variavel global com a mensagem recebiada via TCP
char mensagemTcpOut[64] = "0"; //ultima mensagem enviada via TCP
int valorRecebido = 1; //armazena o valor recebido via TCP em um int
char mode = 'a'; //modo de saida selecionado

//variaveis e funcoes de teste
// int count = 0; //contagem para teste do loop principal 
// void taskLoopCode(void * parameters);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//SETUP e LOOP
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  //setup dos pinos
  pinMode(RUNADDR, OUTPUT); 
  pinMode(BIT1ADDR, OUTPUT);
  pinMode(BIT2ADDR, OUTPUT);
  pinMode(BIT3ADDR, OUTPUT);
  digitalWrite(RUNADDR, LOW);
  digitalWrite(BIT1ADDR, LOW);
  digitalWrite(BIT2ADDR, LOW);
  digitalWrite(BIT3ADDR, LOW);
  pinMode (SPI_CS, OUTPUT); 
  //wireless
  if(!WiFi.config(local_IP, gateway, subnet)) { //configura o ip estatico
    Serial.println("falha na configuracao do WiFi!!");
  }
  connectWiFi(); //conecta wifi, posteriormente a conecção é mantida via task.
  delay(100);
  sv.begin(); // inicia o server para o socket
  //setup para OTA update
  ArduinoOTA.setHostname("Ventilador");
  // No authentication by default
  // ArduinoOTA.setPassword("admin");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  // funcao que inicia as tasks
  bootSequence(); 
}

void loop() {
  vTaskDelete(NULL); //não utiliza. as tasks lançadas no bootSequence rodam em loop.  
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//TASKS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void taskCheckConnCode (void * parameters) {
  for (;;) {
    if (WiFi.status() == WL_CONNECTED){
      vTaskDelay(PERIODO/portTICK_PERIOD_MS);
      ArduinoOTA.handle();
      continue;
    }
    connectWiFi();
  }
}



void taskTcpCode(void * parameters) {
  for (;;) {
    if (cl.connected()) {
        if (cl.available() > 0) {
          int i = 0;
          char bufferEntrada[64] = "";
          while (cl.available() > 0) {
            char z = cl.read();
            bufferEntrada[i] = z;
            i++;
            if(z=='\r') {
              bufferEntrada[i] = '\0';
              i++;
            }
          }
          strncpy(mensagemTcpIn,bufferEntrada,i);
          if(closeAfterRec) {
            cl.stop();
          }
          checkValue();
        }
    }
    else {
        // Serial.print("   nenhum cliente conectado...");
        cl = sv.available();//Disponabiliza o servidor para o cliente se conectar.
        vTaskDelay(1000/portTICK_PERIOD_MS);
        Serial.println("conn offline");
    }
  }
}

void taskRPMCode (void * parameters) {
  if (mode == 'a') {
    RPMAnalogico();
    vTaskDelete(NULL);
  }
  else if (mode == 'd') {
    RPMDigital();
    vTaskDelete(NULL);
  }

}

// void taskLoopCode (void * parameters) {
//   for (;;) {
//     Serial.print("\ncontagem: ");
//     Serial.print(count);
//     count++;
//     Serial.print("  ultimo valor recebido: ");
//     Serial.print(valorRecebido);
//     Serial.print("  modo atual:");
//     Serial.print(mode);
//     Serial.print("  mensagemTcpIn: ");
//     Serial.print(mensagemTcpIn);
//     Serial.print("\n  valor de atoi: ");
//     Serial.print(atoi(mensagemTcpIn));    
//     vTaskDelay(2000/portTICK_PERIOD_MS);
//   }
// }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Funções
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void bootSequence (){
  delay(2000);
  // Serial.println("criando task de reconexao...");
  xTaskCreatePinnedToCore(taskCheckConnCode,"conexao wifi",5000,NULL,1,&taskCheckConn,CONFIG_ARDUINO_RUNNING_CORE);
  //Serial.println("criando task de loop...");
  //xTaskCreatePinnedToCore(taskLoopCode,"task principal",1000,NULL,1,&taskLoop,coreTask);
  // Serial.println("criando task de TCP...");
  xTaskCreatePinnedToCore(taskTcpCode,"task TCP",2000,NULL,1,&taskTcp,CONFIG_ARDUINO_RUNNING_CORE);
  pot();
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  WiFi.begin(SSID, PASS);
  // Serial.print("Conectando ao WiFi.");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    // Serial.println(".");
  }
  // Serial.println("Connectado ao WiFi!!");
  // Serial.print("IP:");
  // Serial.println(WiFi.localIP()); 
}

void checkValue() {
  for(int y=0;y < strlen(mensagemTcpIn);y++){
         Serial.println(mensagemTcpIn[y],DEC);
         Serial.println(mensagemTcpIn[y]);
  }
  if (strcmp(mensagemTcpIn,"a\r")==0){
    mensagemTcpOut[0] = '2';
    cl.print(mensagemTcpOut);
    mode = 'a';
    // Serial.println("\n\nAlterando para saida analogica");
  }
  else if (strcmp(mensagemTcpIn,"d\r")==0){
    mensagemTcpOut[0] = '2';
    cl.print(mensagemTcpOut);
    mode = 'd';
    // Serial.println("\n\nAlterando para saida digital");
  }
  else if (strcmp(mensagemTcpIn,"s\r")==0){
    mensagemTcpOut[0] = '4';
    cl.print(mensagemTcpOut);
    RPMStop();
    // Serial.println("\n\nParando o ventilador");
  }
  else if (strcmp(mensagemTcpIn,"r\r")==0){
    mensagemTcpOut[0] = '5';
    cl.print(mensagemTcpOut);
    RPMStart();
    // Serial.println("\n\nIniciando o ventilador");
  }
  else if(atoi(mensagemTcpIn)<9 && atoi(mensagemTcpIn)>0 && mode == 'd'){
    valorRecebido = atoi(mensagemTcpIn);
    mensagemTcpOut[0] = '0';
    cl.print(mensagemTcpOut);
    xTaskCreatePinnedToCore(taskRPMCode,"task RPM",1000,NULL,1,&taskRPM,coreTask);
    // Serial.println("\n\nAlterando valor na saida digital: ");
    // Serial.println(valorRecebido);
  }
  else if (atoi(mensagemTcpIn)<257 && atoi(mensagemTcpIn)>0 && mode == 'a'){
    valorRecebido = atoi(mensagemTcpIn);
    mensagemTcpOut[0] = '0';
    cl.print(mensagemTcpOut);
    xTaskCreatePinnedToCore(taskRPMCode,"task RPM",1000,NULL,1,&taskRPM,coreTask);
    // Serial.print("\n\nalterando valor na saida analogica: ");
    // Serial.println(valorRecebido);
  }
  else {
      mensagemTcpOut[0] = '1';
      cl.print(mensagemTcpOut);
      //Serial.println("\nerro: comando inexistente ou valor fora da faixa");
       for(int y=0;y < strlen(mensagemTcpIn);y++){
         Serial.println(mensagemTcpIn[y],DEC);
         Serial.println(mensagemTcpIn[y]);
       }
  }
}

void RPMAnalogico() {
  pot();
}

void RPMDigital() {
  if(valorRecebido == 1 ) {
    digitalWrite(BIT1ADDR, LOW);
    digitalWrite(BIT2ADDR, LOW);
    digitalWrite(BIT3ADDR, LOW);
  }
  else if(valorRecebido == 2 ) {
    digitalWrite(BIT1ADDR, LOW);
    digitalWrite(BIT2ADDR, LOW);
    digitalWrite(BIT3ADDR, HIGH);
  }
  else if(valorRecebido == 3 ) {
    digitalWrite(BIT1ADDR, LOW);
    digitalWrite(BIT2ADDR, HIGH);
    digitalWrite(BIT3ADDR, LOW);
  }
  else if(valorRecebido == 4 ) {
    digitalWrite(BIT1ADDR, LOW);
    digitalWrite(BIT2ADDR, HIGH);
    digitalWrite(BIT3ADDR, HIGH);
  }
  else if(valorRecebido == 5 ) {
    digitalWrite(BIT1ADDR, HIGH);
    digitalWrite(BIT2ADDR, LOW);
    digitalWrite(BIT3ADDR, LOW);
  }
  else if(valorRecebido == 6 ) {
    digitalWrite(BIT1ADDR, HIGH);
    digitalWrite(BIT2ADDR, LOW);
    digitalWrite(BIT3ADDR, HIGH);
  }
  else if(valorRecebido == 7 ) {
    digitalWrite(BIT1ADDR, HIGH);
    digitalWrite(BIT2ADDR, HIGH);
    digitalWrite(BIT3ADDR, LOW);
  }
  else if(valorRecebido == 8 ) {
    digitalWrite(BIT1ADDR, HIGH);
    digitalWrite(BIT2ADDR, HIGH);
    digitalWrite(BIT3ADDR, HIGH);
  }
}

void RPMStart(){
  digitalWrite(RUNADDR, HIGH);
}

void RPMStop(){
  digitalWrite(RUNADDR, LOW);
}

void RPMCode () {
  if (mode == 'a') {
    RPMAnalogico();
  }
  else if (mode == 'd') {
    RPMDigital();
  }

}

void pot() {
  digitalWrite(SPI_CS, LOW);
  SPI.transfer(0x11);
  SPI.transfer(valorRecebido-1);
  digitalWrite(SPI_CS, HIGH);
}

void printTest(){

}