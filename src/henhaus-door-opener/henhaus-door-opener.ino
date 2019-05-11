
// Definice promennych

#define MODE_AUTO 0x00
#define MODE_MANUAL 0x01
#define NAHORU 0x01
#define DOLU 0x00
#define STOP 0x02

// Nastaveni relatek
#define RELE_ON LOW
#define RELE_OFF HIGH

// Nastaveni spinacu pro dojezd1 - nahore
#define KONTAKT_DOJEZD1_ON HIGH
#define KONTAKT_DOJEZD1_OFF LOW

// Nastaveni spinacu pro DOJEZD2 (dole kontakt)
#define KONTAKT_DOJEZD2_ON LOW
#define KONTAKT_DOJEZD2_OFF HIGH

#define TLACITKO_ON LOW
#define TLACITKO_OFF HIGH

// Definice prahovych promennych
const int prah_svetla = 1020;  // stupne ADC
const int prah_motor_nadproud = 500; // stupne ADC
unsigned long motorStopTime = 50;   // Doba zastaveni motoru pokud nezareaguji vypinace
const int zpozdeniDetekceSvetla = 30; // sekundy

// Pomocne konstanty a promenne
unsigned long puvodniHodnotaZpozdeniSvetla = 0;
unsigned long startMotorStopZpozdeni = 0;
unsigned long oldLEDint = 0;        // will store last time LED was updated
bool stavDen = false;
bool aktualniStavSvetla = false;
bool posledniStavSvetla = false;
int ledStatus = LOW;             // ledState used to set the LED
int oldMode = MODE_AUTO;
int firstTime = true;
unsigned long stopTime = 0;


// Definice vstupne vystupnich rozhrani
const int DOJEZD1 = 2;
const int DOJEZD2 = 3;
const int TLACITKO_NAHORU = 4;
const int TLACITKO_DOLU = 5;
const int PREPINAC_MODU = 6;
const int FOTOREZISTOR = A0;
const int NADPROUD = A1;
const int RELE1 = 7;
const int RELE2 = 8;
const int RELE_LED = LED_BUILTIN; // Interni LED dioda

int statusDOJEZD1 = LOW;    // kontakt pohybu nahore
int statusDOJEZD2 = LOW;    // kontakt pohybu dole (jazyckove rele)
int statusTLACITKO_NAHORU = LOW;
int statusTLACITKO_DOLU = LOW;
int statusPREPINAC_MODU = LOW;
int buttonNAHORU = LOW;
int buttonDOLU = LOW;
int buttonMOD = LOW;

void blikLED(int ledPin, int interval) {

  unsigned long LEDint = millis();

  if (LEDint - oldLEDint >= interval) {
    // save the last time you blinked the LED
    oldLEDint = LEDint;

    // if the LED is off turn it on and vice-versa:
    if (ledStatus == LOW) {
      ledStatus = HIGH;
    } else {
      ledStatus = LOW;
    }

    // set the LED with the ledState of the variable:
    digitalWrite(ledPin, ledStatus);

  }
}

void stopLED(int ledPin) {

  // Zhasnuti LED
  digitalWrite(ledPin, LOW);
}

int checkDenNoc(int zpozdeni) {

  // Zmena svetla muze probehnout jen po zadanem intervalu v sekundach.

  unsigned long hodnotaZpozdeniSvetla = millis();

  if (hodnotaZpozdeniSvetla - puvodniHodnotaZpozdeniSvetla >= (zpozdeni * 1000)) {

    puvodniHodnotaZpozdeniSvetla = hodnotaZpozdeniSvetla;

    int sensorValue = analogRead(FOTOREZISTOR);
    float sensorVoltage = sensorValue * (5.0 / 1023);

    if (sensorValue >= prah_svetla) {
      Serial.print("Detekovana NOC: ");
      stavDen = false;
    } else {
      Serial.print("Detekovan DEN: ");
      stavDen = true;
    }

    Serial.print(sensorVoltage);
    Serial.print(" V (");
    Serial.print(sensorValue);
    Serial.println(") ");
  }

  return stavDen;
}


void modKalibrace() {

  Serial.println("Rezim kalibrace");

  // Dvere jedou nahoru
  motorAktivace(NAHORU);

  // Dvere jedou dolu
  motorAktivace(DOLU);

  if (checkDenNoc(0) == true) {
    // Dvere jedou nahoru
    motorAktivace(NAHORU);
  } else {
    // Dvere jedou nahoru
    motorAktivace(DOLU);
  }
}

// Funkce pro volbu smeru toceni motoru
bool motorPohyb(int smer) {
  if (smer == NAHORU) {
    // Spusteni motoru nahoru
    Serial.println("Motor jede NAHORU");

    // RELE1 = OFF, RELE2 = ON
    digitalWrite(RELE1, RELE_OFF);
    digitalWrite(RELE2, RELE_ON);

  } else if (smer == DOLU) {
    // Spusteni motoru dolu
    Serial.println("Motor jede DOLU");

    // RELE1 = ON, RELE2 = OFF
    digitalWrite(RELE1, RELE_ON);
    digitalWrite(RELE2, RELE_OFF);

  } else if (smer == STOP) {
    // Motor zastaven
    Serial.print("Motor zastaven po ");
    Serial.print(stopTime);
    Serial.println(" ms");

    // RELE1 = OFF, RELE2 = OFF
    digitalWrite(RELE1, RELE_OFF);
    digitalWrite(RELE2, RELE_OFF);
  }
}


// Funkce pro detekci zastaveni motoru nadproudem (motor ve zkratu)
bool nadproudMotor() {

  // Zablokovani funkce ....
  return false;

  int sensorValue = analogRead(NADPROUD);
  float sensorVoltage = sensorValue * (5.0 / 1023);

  // Pokud je motor v chodu nedelam nic
  if (sensorValue <= prah_motor_nadproud) {
    return false;
  } else {
    Serial.print("Detekovan nadproud ");
    Serial.print(sensorVoltage);
    Serial.print(" V (");
    Serial.print(sensorValue);
    Serial.println(") ");
    return true;
  }

}

// Funkce pro aktivaci motoru vcetne cekani na maximalni dobu behu
bool motorAktivace(int smer) {

  if (motorSTATUS() != smer) {
    motorPohyb(smer);
    startMotorStopZpozdeni = millis();

    delay(2000);

    // Cekani na dojeti motoru na pozadovanou pozici.
    while (true) {
      if (motorSTOP() == true) {
        stopLED(RELE_LED);
        return true;
      }
      // Aktivace indikacni led
      blikLED(RELE_LED, 150);
    }

  }

}

int motorSTATUS() {

  int poziceMOTORU = 0x03;

  if ((statusDOJEZD1 == KONTAKT_DOJEZD1_ON) && (statusDOJEZD2 == KONTAKT_DOJEZD2_ON)) {
    poziceMOTORU = NAHORU;
  }

  if ((statusDOJEZD1 == KONTAKT_DOJEZD1_OFF) && (statusDOJEZD2 == KONTAKT_DOJEZD2_OFF)) {
    poziceMOTORU = DOLU;
  }

  return poziceMOTORU;
}


// Funkce pro zastaveni pohybu motoru pri dosazeni hranicnich hodnot
bool motorSTOP() {
  stopTime = millis() - startMotorStopZpozdeni;

  statusDOJEZD1 = digitalRead(DOJEZD1);
  statusDOJEZD2 = digitalRead(DOJEZD2);

  // Zastaveni motoru z duvodu detekce dojezdu nebo pri vyprseni casu
  if (nadproudMotor() == true) {
    Serial.println("Zastaveni motoru nadproudem NAHORU");
    motorPohyb(STOP);
    return true;

// Zastaveni motoru, kdyz jsou dvirka nahore
    } else if ((statusDOJEZD1 == KONTAKT_DOJEZD1_ON) && (statusDOJEZD2 == KONTAKT_DOJEZD2_ON)) {
    Serial.println("Zastaveni motoru NAHORE");
    motorPohyb(STOP);
    return true;

// Zastaveni motoru, kdyz jsou dvirka dole
// Osetreni na agilni slepici. Nastavena mez 30 sekund po kterou cidla nereaguji.
  } else if ((statusDOJEZD1 == KONTAKT_DOJEZD1_OFF) && (statusDOJEZD2 == KONTAKT_DOJEZD2_OFF)) {
    //  } else if ((statusDOJEZD1 == KONTAKT_OFF) && (posledniSTOP == NAHORU)) {
    if (stopTime >= (motorStopTime * 1000 / 1.5)) { 
      Serial.println("Zastaveni motoru DOLE");
      motorPohyb(STOP);
      return true;
    }
    Serial.println("Slepice se dere ven ...");

  } else if (stopTime >= (motorStopTime * 1000)) {
    Serial.println("Zastaveni motoru kvuli prekroceni casu");
    motorPohyb(STOP);
    Serial.println("Uz nejedu dal!!!!!");
    while (true) {
    }
    return true;
  }

  return false;
}

void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);

  // Nastaveni pinu kurniku
  pinMode(DOJEZD1, INPUT_PULLUP);
  pinMode(DOJEZD2, INPUT_PULLUP);
  pinMode(TLACITKO_NAHORU, INPUT_PULLUP);
  pinMode(TLACITKO_DOLU, INPUT_PULLUP);
  pinMode(PREPINAC_MODU, INPUT_PULLUP);
  pinMode(RELE1, OUTPUT);
  pinMode(RELE2, OUTPUT);
  pinMode(RELE_LED, OUTPUT);
}


void loop() {

  // Nacteni stavu pinu tlacitek atd.
  statusPREPINAC_MODU = digitalRead(PREPINAC_MODU);

  // Prepnuti na automaticky rezim
  if (statusPREPINAC_MODU == HIGH) {


    // Detekce prvniho spusteni nebo prepnuti rezimu MANUAL/AUTO
    if ((oldMode == MODE_MANUAL) || (firstTime == true)) {
      Serial.println("Automaticky rezim");
      firstTime = false;
      oldMode = MODE_AUTO;

      // kalibrace dveri pri prvnim spusteni kodu
      modKalibrace();
    }

    // Detekce svetla
    aktualniStavSvetla = checkDenNoc(zpozdeniDetekceSvetla);

    // Pokud se zmenil DEN/NOC tak vyvolam spusteni motoru
    if (aktualniStavSvetla != posledniStavSvetla) {
      posledniStavSvetla = aktualniStavSvetla;

      // Pokud je DEN tak pustim motor dolu
      if (aktualniStavSvetla == true) {
        Serial.println("Je DEN, poustim motor NAHORU");
        motorAktivace(NAHORU);

      } else { // Pokud je NOC tak poustim motor nahoru
        Serial.println("Je NOC, poustim motor DOLU");
        motorAktivace(DOLU);

      }

    }

  } else { // Prepnuti na manualni rezim

//    while (true) {
//      statusDOJEZD1 = digitalRead(DOJEZD1);
//      statusDOJEZD2 = digitalRead(DOJEZD2);
//      Serial.print("DOJEZD1: ");
//      Serial.println(statusDOJEZD1);
//      Serial.print("DOJEZD2: ");
//      Serial.println(statusDOJEZD2);
//      Serial.print("");
//      delay(300);
//    }

    // Detekce prvniho spusteni nebo prepnuti rezimu MANUAL/AUTO
    if ((oldMode == MODE_AUTO) || (firstTime == true)) {
      Serial.println("Manualni rezim");
      oldMode = MODE_MANUAL;
      firstTime = false;
    }

    // Nacteni stavu tlacitek
    statusTLACITKO_NAHORU = digitalRead(TLACITKO_NAHORU);
    statusTLACITKO_DOLU = digitalRead(TLACITKO_DOLU);

    if (statusTLACITKO_NAHORU == TLACITKO_ON) {
      // Manualni spusteni motoru nahoru
      Serial.println("Poustim motor NAHORU");
      motorAktivace(NAHORU);

    } else if (statusTLACITKO_DOLU == TLACITKO_ON) {
      // Manualni spusteni motoru dolu
      Serial.println("Poustim motor DOLU");
      motorAktivace(DOLU);
    }
  }

  //delay(300);
}
