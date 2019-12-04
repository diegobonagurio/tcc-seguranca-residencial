int contato = 2; //definição da porta Digital 2
int pirPin = 6; //definição da porta Digital 2

#include <EEPROM.h>     //Biblioteca para gravar dados das Tag na EEPROM
#include <SPI.h>        // RC522 Module uses SPI protocol Comunicação SPI
#include <MFRC522.h>  // Biblioteca para o RFID
#define BLYNK_PRINT Serial
#include <SPI.h>
#include <Ethernet.h> //Biblioteca para Ethernet Shield
#include <BlynkSimpleEthernet.h>
#include <Servo.h> //Biblioteca para Servo Motor
#include <SerialRelay.h> //Biblioteca para modulo Rele Serial
SerialRelay relays(22, 23, 1); // (data, clock, número de módulos)
#define COMMON_ANODE
#ifdef COMMON_ANODE
#define LED_ON LOW
#define LED_OFF HIGH
#else
#define LED_ON HIGH
#define LED_OFF LOW
#endif
#define redLed 27    // Led Vermelho
#define greenLed 26  //Led Verde
#define blueLed 25   //Led Azul
#define relay 8     // Rele para acionamento da tranca
#define wipeB 3     // Apaga as tag cadastrada
#define W5100_CS  10
#define SDCARD_CS 4

bool programMode = false;  // initialize programming mode to false

uint8_t successRead;    // Varialvel para caso seja leitura bem sucedida

byte storedCard[4];   //Armazena uma TAG lida da EEPROM
byte readCard[4];   // Armazena a Tag lida no leitor
byte masterCard[4];   //Armazena a TAG lido na EEPROM

// Create MFRC522 instance.
#define SS_PIN 53 //Definindo a porta 53 para comunicação RFID
#define RST_PIN 5 //Definindo a porta 53 para comunicação RFID
MFRC522 mfrc522(SS_PIN, RST_PIN);

char auth[] = "645423c360cf4a069f02def59dc81002"; //Auth Token do Projeto no Blynk

BLYNK_WRITE(V0) { //Acionamento do Relé 01
  int pinValue = param.asInt();
  //Serial.println(pinValue);
  if (pinValue == 1) {
    relays.SetRelay(1, SERIAL_RELAY_ON, 1);   // liga o relé 01
  } else {
    relays.SetRelay(1, SERIAL_RELAY_OFF, 1);  // desliga o relé 01
  }
}

BLYNK_WRITE(V1) { //Acionamento do Rele 02
  int pinValue = param.asInt();
  //Serial.println(pinValue);
  if (pinValue == 1) {
    relays.SetRelay(2, SERIAL_RELAY_ON, 1);   // liga o relé 02
  } else {
    relays.SetRelay(2, SERIAL_RELAY_OFF, 1);  // desliga o relé 02
  }
}

Servo servo; //Servo Motor X
Servo servo2; //Servo Motor Y

///////////Movimento do servo motor X////////////////////////
BLYNK_WRITE(V3)
{
  servo.write(param.asInt());
}

/////////////Movimento o Servo Motor Y/////////////////////////
BLYNK_WRITE(V4)
{
  servo2.write(param.asInt());
}

///////////////////////////////////////// Setup ///////////////////////////////////
void setup() {
  //Configuração dos pinos no Arduino
  pinMode(SDCARD_CS, OUTPUT);
  digitalWrite(SDCARD_CS, HIGH); // 
  Blynk.begin(auth);
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  pinMode(wipeB, INPUT_PULLUP);   // Habilita pino resistor pull-up
  pinMode(relay, OUTPUT);
  digitalWrite(relay, HIGH);    
  digitalWrite(redLed, LED_OFF);  
  digitalWrite(greenLed, LED_OFF);  
  digitalWrite(blueLed, LED_OFF); 
  servo.attach(4);
  servo2.attach(7);
  pinMode(contato, INPUT);
  pinMode(pirPin, INPUT);

  //Protocol Configuration
  Serial.begin(9600);  // Initialize serial communications with PC
  SPI.begin();           // Protocolo de comunicação
  mfrc522.PCD_Init();    // Inicializa o leitor RFID

  Serial.println(F("Controle de Acesso Example v0.1"));   

  if (digitalRead(wipeB) == LOW) {  //Botão pressionado ativa a função WIPE
    digitalWrite(redLed, LED_ON); 
    Serial.println(F("Botao WIPE pressionado"));
    Serial.println(F("Você tem 10 segundo para cancelar essa operacao"));
    Serial.println(F("Isso removerá todos os registros e não poderá ser desfeito"));
    bool buttonState = monitorWipeButton(10000); // Delay de 10segundos
    if (buttonState == true && digitalRead(wipeB) == LOW) {    // Se o botão ainda estiver pressionado
      Serial.println(F("Iniciando processo de limpeza da EEPROM"));
      for (uint16_t x = 0; x < EEPROM.length(); x = x + 1) {    //Loop final do endereço da EEPROM
        if (EEPROM.read(x) == 0) {              //Se o endereço EEPROM = 0
        }
        else {
          EEPROM.write(x, 0);       // se não escrever 0 para limpar, leva 3.3mS
        }
      }
      Serial.println(F("Limpado EEPROM"));
      digitalWrite(redLed, LED_OFF); 
      delay(200);
      digitalWrite(redLed, LED_ON);
      delay(200);
      digitalWrite(redLed, LED_OFF);
      delay(200);
      digitalWrite(redLed, LED_ON);
      delay(200);
      digitalWrite(redLed, LED_OFF);
    }
    else {
      Serial.println(F("Limpeza Cancelada")); // Show some feedback that the wipe button did not pressed for 15 seconds
      digitalWrite(redLed, LED_OFF);
    }
  }
  if (EEPROM.read(1) != 143) {
    Serial.println(F("Nenhuma Tag Master Definida"));
    Serial.println(F("Aproxime uma Tag para defini-la como Master"));
    do {
      successRead = getID();            
      digitalWrite(blueLed, LED_ON);    
      delay(200);
      digitalWrite(blueLed, LED_OFF);
      delay(200);
    }
    while (!successRead);                  // O programa ficará lendo até encontrar uma tag
    for ( uint8_t j = 0; j < 4; j++ ) {        // Loop 4 times
      EEPROM.write( 2 + j, readCard[j] );  
    }
    EEPROM.write(1, 143);                  
    Serial.println(F("Definido Tag Master"));
  }
  Serial.println(F("-------------------"));
  Serial.println(F("Tag MasterUID"));
  for ( uint8_t i = 0; i < 4; i++ ) {          
    masterCard[i] = EEPROM.read(2 + i);   
    Serial.print(masterCard[i], HEX);
  }
  Serial.println("");
  Serial.println(F("-------------------"));
  Serial.println(F("Tudo Pronto"));
  Serial.println(F("Esperando PICCs para ser escaneadas:"));
  cycleLeds();   
}


///////////////////////////////////////// Main Loop ///////////////////////////////////
void loop () {
  do {

///////////////////////////////////////Rotina de Verificação de Estado do Sensor Magnético /////////////////////////////////
    Blynk.run();
    if (digitalRead(contato) == HIGH) {
      Blynk.notify("A PORTA FOI ABERTA");
      delay(1000);
    }

///////////////////////////////////////Rotina de Verificação de Estado do Sensor de Presença ///////////////////////////////
    int pirValue = digitalRead(pirPin);
    if (pirValue == HIGH) {
      Blynk.notify("MOVIMENTO DETECTADO");
      delay(1000);
    }
    if (pirValue == LOW) {
      delay(1000);
    }

////////////////////////////////////Rotina de Funcionamento RFID ///////////////////////////////////////////////////////
    successRead = getID();  // sets successRead to 1 when we get read from reader otherwise 0
    if (digitalRead(wipeB) == LOW) { // Confere se o botão WIPE foi pressionado
      digitalWrite(redLed, LED_ON);  
      digitalWrite(greenLed, LED_OFF);  
      digitalWrite(blueLed, LED_OFF);
      Serial.println(F("Botão WIPE Pressionado"));
      Serial.println(F("Master Card será apagado! em 10 segundos"));
      bool buttonState = monitorWipeButton(10000); // Delay 10s
      if (buttonState == true && digitalRead(wipeB) == LOW) {    // Se estiver pressionado, apaga da EEPROM
        EEPROM.write(1, 0);                  // Reset Magic Number.
        Serial.println(F("Master Card apagado do dispositivo"));
        Serial.println(F("Por favor, redefina para reprogramar o Master Card"));
        while (1);
      }
      Serial.println(F("Cancelado processo"));
    }
    if (programMode) {
      cycleLeds();              
    }
    else {
      normalModeOn();     
    }
  }
  while (!successRead);   
  if (programMode) {
    if ( isMaster(readCard) ) { //Quando no modo de programa verificar primeiro Se o cartão mestre for digitalizado novamente para sair do modo de programação
      Serial.println(F("Tag Master lida"));
      Serial.println(F("Saindo do Modo Gravação"));
      Serial.println(F("-----------------------------"));
      programMode = false;
      return;
    }
    else {
      if ( findID(readCard) ) { //  Se o cartão digitalizado for conhecido, exclua-o
        Serial.println(F("Tag já cadastrada, removendo Tag..."));
        deleteID(readCard);
        Serial.println("-----------------------------");
        Serial.println(F("Aproxime uma PICC para ADD ou REMOVE da EEPROM"));
      }
      else {                    // If scanned card is not known add it
        Serial.println(F("Tag não cadastrada, adicionando..."));
        writeID(readCard);
        Serial.println(F("-----------------------------"));
        Serial.println(F("Aproxime uma PICC para ADD ou REMOVE da EEPROM"));
      }
    }
  }
  else {
    if ( isMaster(readCard)) {    // Se o ID do cartão escaneado coincidir com o ID do cartão mestre - entre no modo de programa
      programMode = true;
      Serial.println(F("Bem-Vindo Master ao Modo Gravacao"));
      uint8_t count = EEPROM.read(0);   
      Serial.print(F("Existem "));    
      Serial.print(count);
      Serial.print(F(" registros na EEPROM"));
      Serial.println("");
      Serial.println(F("Aproxime uma PICC para ADD ou REMOVE da EEPROM"));
      Serial.println(F("Aproxime a Tag Master novamente para sair do Modo Gravacao"));
      Serial.println(F("-----------------------------"));
    }
    else {
      if ( findID(readCard) ) { // If not, see if the card is in the EEPROM
        Serial.println(F("Bem-Vindo, acesso permitido"));
        granted(300);        
      }
      else {      
        Serial.println(F("Acesso Negado"));
        denied();
      }
    }
  }
}

/////////////////////////////////////////  Acesso Permitido   ///////////////////////////////////
void granted ( uint16_t setDelay) {
  digitalWrite(blueLed, LED_OFF);   
  digitalWrite(redLed, LED_OFF);  
  digitalWrite(greenLed, LED_ON);   
  digitalWrite(relay, LOW);     // Destrava a Porta
  delay(setDelay);          
  digitalWrite(relay, HIGH);    // Tranca a porta novamente
  delay(1000);           
}

///////////////////////////////////////// Acesso Negado  ///////////////////////////////////
void denied() {
  digitalWrite(greenLed, LED_OFF);  
  digitalWrite(blueLed, LED_OFF);   
  digitalWrite(redLed, LED_ON);   
  delay(1000);
}


///////////////////////////////////////// Get PICC's UID ///////////////////////////////////
uint8_t getID() {
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //Se for identificado uma nova tag
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {  
    return 0;
  }
  Serial.println(F("Lido PICC's UID:"));
  for ( uint8_t i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.print(readCard[i], HEX);
  }
  Serial.println("");
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}

void ShowReaderDetails() {
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print(F("MFRC522 Software Version: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91)
    Serial.print(F(" = v1.0"));
  else if (v == 0x92)
    Serial.print(F(" = v2.0"));
  else
    Serial.print(F(" (unknown),probably a chinese clone?"));
  Serial.println("");

  if ((v == 0x00) || (v == 0xFF)) {
    Serial.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
    Serial.println(F("SYSTEM HALTED: Check connections."));
    // Visualize system is halted
    digitalWrite(greenLed, LED_OFF);  // Make sure green LED is off
    digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
    digitalWrite(redLed, LED_ON);   // Turn on red LED
    while (true); // do not go further
  }
}

///////////////////////////////////////// Ciclo dos LED's ///////////////////////////////////
void cycleLeds() {
  digitalWrite(redLed, LED_OFF);  // Make sure red LED is off
  digitalWrite(greenLed, LED_ON);   // Make sure green LED is on
  digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
  delay(200);
  digitalWrite(redLed, LED_OFF);  // Make sure red LED is off
  digitalWrite(greenLed, LED_OFF);  // Make sure green LED is off
  digitalWrite(blueLed, LED_ON);  // Make sure blue LED is on
  delay(200);
  digitalWrite(redLed, LED_ON);   // Make sure red LED is on
  digitalWrite(greenLed, LED_OFF);  // Make sure green LED is off
  digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
  delay(200);
}

//////////////////////////////////////// Normal Mode Led  ///////////////////////////////////
void normalModeOn () {
  digitalWrite(blueLed, LED_ON); 
  digitalWrite(redLed, LED_OFF);  
  digitalWrite(greenLed, LED_OFF);  
  digitalWrite(relay, HIGH);    
}

//////////////////////////////////////// Busca uma Tag na EEPROM //////////////////////////////
void readID( uint8_t number ) {
  uint8_t start = (number * 4 ) + 2;   
  for ( uint8_t i = 0; i < 4; i++ ) {     
    storedCard[i] = EEPROM.read(start + i);  
  }
}

///////////////////////////////////////// Adicionar TAG a EEPROM  ///////////////////////////////////
void writeID( byte a[] ) {
  if ( !findID( a ) ) {     // Before we write to the EEPROM, check to see if we have seen this card before!
    uint8_t num = EEPROM.read(0);     // Get the numer of used spaces, position 0 stores the number of ID cards
    uint8_t start = ( num * 4 ) + 6;  // Figure out where the next slot starts
    num++;                // Increment the counter by one
    EEPROM.write( 0, num );     // Write the new count to the counter
    for ( uint8_t j = 0; j < 4; j++ ) {   // Loop 4 times
      EEPROM.write( start + j, a[j] );  // Write the array values to EEPROM in the right position
    }
    successWrite();
    Serial.println(F("Registro de ID adicionado com sucesso à EEPROM"));
  }
  else {
    failedWrite();
    Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
  }
}

///////////////////////////////////////// Remove TAG da EEPROM   ///////////////////////////////////
void deleteID( byte a[] ) {
  if ( !findID( a ) ) {     // Before we delete from the EEPROM, check to see if we have this card!
    failedWrite();      // If not
    Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
  }
  else {
    uint8_t num = EEPROM.read(0);   // Get the numer of used spaces, position 0 stores the number of ID cards
    uint8_t slot;       // Figure out the slot number of the card
    uint8_t start;      // = ( num * 4 ) + 6; // Figure out where the next slot starts
    uint8_t looping;    // The number of times the loop repeats
    uint8_t j;
    uint8_t count = EEPROM.read(0); // Read the first Byte of EEPROM that stores number of cards
    slot = findIDSLOT( a );   // Figure out the slot number of the card to delete
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--;      // Decrement the counter by one
    EEPROM.write( 0, num );   // Write the new count to the counter
    for ( j = 0; j < looping; j++ ) {         // Loop the card shift times
      EEPROM.write( start + j, EEPROM.read(start + 4 + j));   // Shift the array values to 4 places earlier in the EEPROM
    }
    for ( uint8_t k = 0; k < 4; k++ ) {         // Shifting loop
      EEPROM.write( start + j + k, 0);
    }
    successDelete();
    Serial.println(F("Registro de ID removido com sucesso da EEPROM"));
  }
}

///////////////////////////////////////// Check Bytes   ///////////////////////////////////
bool checkTwo ( byte a[], byte b[] ) {
  for ( uint8_t k = 0; k < 4; k++ ) {   // Loop 4 times
    if ( a[k] != b[k] ) {     // IF a != b then false, because: one fails, all fail
      return false;
    }
  }
  return true;
}

///////////////////////////////////////// Find Slot   ///////////////////////////////////
uint8_t findIDSLOT( byte find[] ) {
  uint8_t count = EEPROM.read(0);       // Read the first Byte of EEPROM that
  for ( uint8_t i = 1; i <= count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);                // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      // is the same as the find[] ID card passed
      return i;         // The slot number of the card
    }
  }
}

///////////////////////////////////////// Encontra a ID na EEPROM  ///////////////////////////////////
bool findID( byte find[] ) {
  uint8_t count = EEPROM.read(0);     // Read the first Byte of EEPROM that
  for ( uint8_t i = 1; i < count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);          // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      return true;
    }
    else {    // If not, return false
    }
  }
  return false;
}

///////////////////////////////////////// Escrito com sucesso na EEPROM   ///////////////////////////////////
void successWrite() {
  digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
  digitalWrite(redLed, LED_OFF);  // Make sure red LED is off
  digitalWrite(greenLed, LED_OFF);  // Make sure green LED is on
  delay(200);
  digitalWrite(greenLed, LED_ON);   // Make sure green LED is on
  delay(200);
  digitalWrite(greenLed, LED_OFF);  // Make sure green LED is off
  delay(200);
  digitalWrite(greenLed, LED_ON);   // Make sure green LED is on
  delay(200);
  digitalWrite(greenLed, LED_OFF);  // Make sure green LED is off
  delay(200);
  digitalWrite(greenLed, LED_ON);   // Make sure green LED is on
  delay(200);
}

///////////////////////////////////////// Erro ao gravar na EEPROM   ///////////////////////////////////
void failedWrite() {
  digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
  digitalWrite(redLed, LED_OFF);  // Make sure red LED is off
  digitalWrite(greenLed, LED_OFF);  // Make sure green LED is off
  delay(200);
  digitalWrite(redLed, LED_ON);   // Make sure red LED is on
  delay(200);
  digitalWrite(redLed, LED_OFF);  // Make sure red LED is off
  delay(200);
  digitalWrite(redLed, LED_ON);   // Make sure red LED is on
  delay(200);
  digitalWrite(redLed, LED_OFF);  // Make sure red LED is off
  delay(200);
  digitalWrite(redLed, LED_ON);   // Make sure red LED is on
  delay(200);
}

///////////////////////////////////////// Concluido para remover Tag da EEPROM  ///////////////////////////////////
void successDelete() {
  digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
  digitalWrite(redLed, LED_OFF);  // Make sure red LED is off
  digitalWrite(greenLed, LED_OFF);  // Make sure green LED is off
  delay(200);
  digitalWrite(blueLed, LED_ON);  // Make sure blue LED is on
  delay(200);
  digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
  delay(200);
  digitalWrite(blueLed, LED_ON);  // Make sure blue LED is on
  delay(200);
  digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
  delay(200);
  digitalWrite(blueLed, LED_ON);  // Make sure blue LED is on
  delay(200);
}

////////////////////// Verifique o cartão de leitura IF é masterCard  ///////////////////////////////////
bool isMaster( byte test[] ) {
  return checkTwo(test, masterCard);
}

bool monitorWipeButton(uint32_t interval) {
  uint32_t now = (uint32_t)millis();
  while ((uint32_t)millis() - now < interval)  {
    // check on every half a second
    if (((uint32_t)millis() % 500) == 0) {
      if (digitalRead(wipeB) != LOW)
        return false;
    }
  }
  return true;
}
