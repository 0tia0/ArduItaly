#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <string.h>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                              CONFIGURAZIONI                                              //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

int mp3_rx = 11, mp3_tx = 10;
int mp3_volume = 20;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                 VARIABILI                                                //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define ADS1_ADDRESS 0x48
#define ADS2_ADDRESS 0x49
#define ADS3_ADDRESS 0x4A
#define ADS4_ADDRESS 0x4B

Adafruit_ADS1115 ads[4];

typedef struct {
    int index;

    char nome[10 + 1];
    int pin;
    int modulo = -1;

    int audio;
} citta;

citta listaCitta[20];

citta Aosta;        citta Milano;       citta Trento;       citta Torino;       citta Venezia;
citta Trieste;      citta Genova;       citta Bologna;      citta Firenze;      citta Perugia;
citta Ancona;       citta Roma;         citta Aquila;       citta Cambobasso;   citta Napoli;
citta Potenza;      citta Bari;         citta Palermo;      citta Cagliari; 

int rangeStart = 0;
int rangeEnd = 1;

SoftwareSerial mp3(mp3_rx, mp3_tx); // RX, TX
DFRobotDFPlayerMini player;

bool isMusicPlaying = false;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                             INIZIALIZZAZIONE                                             //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void inizializzaCitta() {
  sprintf(listaCitta[0].nome, "Aosta"); listaCitta[0].pin = A0; listaCitta[0].modulo = -1;
  sprintf(listaCitta[1].nome, "Milano"); listaCitta[1].pin = A1; listaCitta[1].modulo = -1;
  sprintf(listaCitta[2].nome, "Trento"); listaCitta[2].pin = A2; listaCitta[2].modulo = -1;
  sprintf(listaCitta[3].nome, "Torino"); listaCitta[3].pin = A3; listaCitta[3].modulo = -1;

  listaCitta[0].index = 0;
  listaCitta[1].index = 1;
  listaCitta[2].index = 2;
  listaCitta[3].index = 3;

  
  listaCitta[0].audio = 1;
  listaCitta[1].audio = 2;
  listaCitta[2].audio = 3;
  listaCitta[3].audio = 4;

  // Città assegnate ai moduli ADS1115 (4 moduli x 4 pin ciascuno)
  int pinIndex = 0;
  int moduloIndex = 0;
  char* nomiCitta[] = {"Venezia", "Trieste", "Genova", "Bologna", "Firenze", "Perugia", "Ancona", "Roma",
                       "Aquila", "Campobasso", "Napoli", "Potenza", "Bari", "Catanzaro", "Palermo", "Cagliari"};

  for (int i = 4; i < 20; i++) {
      listaCitta[i].index = i;
      listaCitta[i].audio = i+1;

      sprintf(listaCitta[i].nome, "%s", nomiCitta[i - 4]);

      listaCitta[i].pin = pinIndex;
      listaCitta[i].modulo = moduloIndex;

      pinIndex++;

      if (pinIndex >= 4) {
          pinIndex = 0;
          moduloIndex++;
      }
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                  EEPROM                                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void changeEEPROMValue(int posizione, int valore) {
    EEPROM.write(posizione, valore);
}

int readEEPROMValue(int posizione) {
    return EEPROM.read(posizione);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                          LETTURA FOTO RESISTENZE                                         //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

int readPhotoResistor(int pin, int modulo = -1) {
    int value = 0;
  
    // Se modulo è -1, significa che il pin è uno dei pin analogici di Arduino (A0, A1, ...)
    if (modulo == -1) {
      value = analogRead(pin);  // Legge direttamente dal pin analogico
    } else {
      // Se modulo è diverso da -1, significa che si sta leggendo da un ADS1115
      if (modulo < 0 || modulo > 3) {
        // Ritorna errore se il modulo non è valido
        return -1;
      }
  
      // Leggi il valore dal canale specificato del modulo corrispondente
      value = ads[modulo].readADC_SingleEnded(pin);
    }
  
    // Ritorna un valore compreso fra 0 e 1023
    return map(value, 0, 4095, 0, 1023);
}

int readFromCity(citta city) {
  return readPhotoResistor(city.pin, city.modulo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                               CALIBRAZIONE                                               //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void calibratePhotoResistor() {
    int primaCitta = random(0, 20);
    int secondaCitta = random(0, 20);
    int terzaCitta = random(0, 20);

    int valorePrimaCitta = readFromCity(listaCitta[primaCitta]);

    int valoreSecondaCitta = readFromCity(listaCitta[secondaCitta]);
    
    int valoreTerzaCitta = readFromCity(listaCitta[terzaCitta]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                  AUDIO                                                   //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void playAudio(int audio) {
  player.play(audio);
}

void playCity(citta city) {
  playAudio(city.audio);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                  TESTO                                                   //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void stampaTabellaCitta() {
  Serial.println(F("Indice\tNome\t\tModulo\tPin"));
  Serial.println(F("------\t----\t\t------\t---"));

  for (int i = 0; i < 20; i++) {
    Serial.print(listaCitta[i].index);
    Serial.print("\t");

    // Aggiusta il padding per nomi più corti
    Serial.print(listaCitta[i].nome);
    int lunghezzaNome = strlen(listaCitta[i].nome);
    if (lunghezzaNome < 8) Serial.print("\t"); // tab extra se nome corto
    Serial.print("\t");

    Serial.print(listaCitta[i].modulo);
    Serial.print("\t");

    Serial.println(listaCitta[i].pin);
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                  MAIN                                                    //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(9600);
  Wire.begin();

  mp3.begin(9600);

  Serial.println("Buongiorno!");
  delay(1000);

  if (!ads[0].begin(ADS1_ADDRESS)) {
    Serial.println("❌ Impossibile trovare ADS1115 con indirizzo 0x48");
    while (1);
  } else {
    Serial.println("✅ Modulo ADS1115 con indirizzo 0x48 inizializzato");
  }
  if (!ads[1].begin(ADS2_ADDRESS)) {
    Serial.println("❌ Impossibile trovare ADS1115 con indirizzo 0x49");
    while (1);
  } else {
    Serial.println("✅ Modulo ADS1115 con indirizzo 0x49 inizializzato");
  }
  if (!ads[2].begin(ADS3_ADDRESS)) {
    Serial.println("❌ Impossibile trovare ADS1115 con indirizzo 0x4A");
    while (1);
  } else {
    Serial.println("✅ Modulo ADS1115 con indirizzo 0x4A inizializzato");
  }
  if (!ads[3].begin(ADS4_ADDRESS)) {
    Serial.println("❌ Impossibile trovare ADS1115 con indirizzo 0x4B");
    while (1);
  } else {
    Serial.println("✅ Modulo ADS1115 con indirizzo 0x4B inizializzato");
  }

  if (player.begin(mp3)) {
    Serial.println("✅ Modulo MP3 inizializzato");
    player.volume(mp3_volume);
  } else {
    Serial.println("❌ Impossibile inizializzare il modulo mp3");
  }  

  inizializzaCitta();
  stampaTabellaCitta();

  listaCitta[0].modulo = 2;
  listaCitta[0].pin = 3;
  listaCitta[0].audio = 3;
}

/* void loop() {
  delay(500);

  for(int i=0; i<20; i++) {
    if(readFromCity(listaCitta[i]) < 5000 && !isMusicPlaying) {
      isMusicPlaying = true;
      playCity(listaCitta[1]);
      break;
    }
  
    if(readFromCity(listaCitta[i]) > 5000 && isMusicPlaying) {
      isMusicPlaying = false;
      player.stop();
      break;
    }
  }

}
 */

 citta* currentCityPlaying = nullptr;

void loop() {
  delay(500);

  if (isMusicPlaying && currentCityPlaying != nullptr) {
    // Controlla solo la città attualmente in riproduzione
    int valore = readFromCity(*currentCityPlaying);
    if (valore > 5000) {
      player.stop();
      Serial.print("🛑 Stop audio da: ");
      Serial.println(currentCityPlaying->nome);

      isMusicPlaying = false;
      currentCityPlaying = nullptr;
    }
    return;
  }

  // Se nessuna musica sta suonando, controlla tutte le città
  for (int i = 0; i < 1; i++) {
    if (readFromCity(listaCitta[i]) < 5000) {
      isMusicPlaying = true;
      currentCityPlaying = &listaCitta[i];

      Serial.print("▶️ Riproduco audio da: ");
      Serial.println(currentCityPlaying->nome);

      playCity(*currentCityPlaying);
      break;
    }
  }
}
