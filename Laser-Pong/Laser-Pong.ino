/* Importieren der benötigten Bibliotheken */
#include <DigitShield.h>
#include <Servo.h>

/* Definitionen der Pins */
const int PIN_BUTTON_SPIELER_1 = 8;
const int PIN_BUTTON_SPIELER_2 = 9;
const int PIN_BUTTON_START = 10;
const int PIN_VCC = 11;

const int PIN_SERVO = A5;

/* Objekt zum Steuern des Servos */
Servo servo;

/* Variablen der einzelnen Spieler */
byte punkteSpieler1;
byte punkteSpieler2;
int ausPositionSpieler1;
int ausPositionSpieler2;
boolean spieler1Blockiert = false;
boolean spieler2Blockiert = false;

/* Variablen des Spiels */
const long wartezeitVorRunde = 2000;
const byte punkteSpielende = 15;
boolean spielLaeuft = false;
long naechsterRundenStart = 0;
const int breiteAktivZone = 7;

/* Variablen für die Steuerung der Servos */
const int startZeitProSchritt = 30;
int zeitProSchritt = startZeitProSchritt;
int ballwechsel = 0;
int servoPosition = 90;
int servoRichtung = 0;
long letzterServoSchritt = 0;

/* Variablen für den Kalibrierungsmodus */
boolean kalibriert = false;
boolean spieler1Kalibriert = false;
boolean spieler2Kalibriert = false;

/*
 * Die setup()-Funktion wird nur einmal beim Start durchlaufen.
 * Dort wird alles gemacht, was nur einmal gemacht werden soll.
 */
void setup() {
  /*
   * Initialisiere das Display
   */
  DigitShield.begin();
  Serial.begin(9600);
  /*
   * Die Pins, an denen die Buttons angeschlossen werden,
   * müssen als Input Pins konfiguriert werden
   */
  pinMode(PIN_BUTTON_SPIELER_1, INPUT); 
  pinMode(PIN_BUTTON_SPIELER_2, INPUT); 
  pinMode(PIN_BUTTON_START, INPUT); 
  
  /*
   * Wir "missbrauchen" einen Pin um die 5V Spannung für die 
   * Taster bereitzustellen, also als OUTPUT konfigurieren 
   * und HIGH Pegel ausgeben
   */
  pinMode(PIN_VCC, OUTPUT);
  digitalWrite(PIN_VCC, HIGH);

  /*
   * Zuletzt noch den PIN an dem der Servo angeschlossen ist
   * setzen
   */
  servo.attach(PIN_SERVO);
}

/*
 * Die loop()-Funktion wird immer wieder durchlaufen.
 */
void loop() { 
  
  if(kalibriert) {
    /*
     * Wenn das Spiel schon kalibriert ist
     */
    if(spielLaeuft) { 
      /*
       * das Spiel wird fortgesetzt, wenn es läuft
       */
      spiel();
      
    } else {
      /*
       * Wenn es nicht läuft, befindet sich das Spiel im Leerlauf
       */
      leerlauf();
    }
  } else {
    /*
     * Ansonsten müssen wir das Spiel noch kalibrieren
     */
    kalibriere(); 
  }
  
  /* Hole eine Zufallszahl */
  random();

  /*
   * Am Schluss müssen wir jedesmal die Anzeige aktualisieren
   */
  aktualisiereAnzeige();
}

/*
 * Wird aufgerufen, um zu entscheiden, was in einem Spiel
 * grade passiert
 */
void spiel() {
  /* Merken der aktuellen Zeit */
  long aktuelleZeit = millis();
  
  /* Wenn der Laser hinter der Position von Spieler 1 ist */
  if(servoPosition > ausPositionSpieler1 + breiteAktivZone) {
    /* Punkt für Spieler 2 und neue Runde */
    ++punkteSpieler2;
    starteNeueRunde();
  }
  
  /* Wenn der Laser hinter der Position von Spieler 2 ist */
  if(servoPosition < ausPositionSpieler2 - breiteAktivZone) {
    /* Punkt für Spieler 1 und neue Runde */
    ++punkteSpieler1;
    starteNeueRunde();
  }
  
  if(spieler1ButtonGedrueckt() && servoRichtung == 1) {
    /* 
     * Wenn Spieler 1 den Button gedrückt hat und der 
     * Lichtpunkt sich in seine Richtung bewegt
     */
    if(spieler1Aktiv() && ! spieler1Blockiert) {
      /* Ändere die Richtung, wenn nicht blockiert */
      servoRichtung = -1;
      ++ballwechsel;
      beschleunige();
    } else {
      /* Blockiere Spieler 1 wenn er zu früh drückt */
      spieler1Blockiert = true;
    }
  } else if(spieler2ButtonGedrueckt() && servoRichtung == -1) {
    /* 
     * Wenn Spieler 1 den Button gedrückt hat und der 
     * Lichtpunkt sich in seine Richtung bewegt
     */
    if(spieler2Aktiv() && ! spieler2Blockiert) {
      /* Ändere die Richtung, wenn nicht blockiert */
      servoRichtung = 1;
      ++ballwechsel;
      beschleunige();
    } else {
      /* Blockiere Spieler 1 wenn er zu früh drückt */
      spieler2Blockiert = true; 
    }
    
  } else if(startButtonGedrueckt()) {
    /* Wenn Startbutton gedrückt wurde */
    delay(500);
    if(startButtonGedrueckt()) {
      /* 
       * Wenn nach einer halben Sekunde immer noch
       * Start gedrückt ist, starte eine neue Kalibrierung
       */
      while(startButtonGedrueckt()) {
        kalibriert = false;
        spieler1Kalibriert = false;
        spieler2Kalibriert = false;
      }
    } else {
      /* 
       * Anderenfalls starte eine neue Runde
       */
      punkteSpieler1 = 0;
      punkteSpieler2 = 0;
      starteNeueRunde();
    }
  }
  /* Wenn ein Spieler genug Punkte hat ist Spielende */
  if(punkteSpieler1 == punkteSpielende || punkteSpieler2 == punkteSpielende) {
    spielLaeuft = false;
  }
   
  /*
   * Der Servo wird nur bewegt, wenn der Rundenstart vergangen ist
   */
  if(naechsterRundenStart < aktuelleZeit) {
    bewegeServo();
  }
}

/*
 * Wird aufgerufen, wenn grade kein Spiel läuft
 */
void leerlauf() {
  if(startButtonGedrueckt()) {
    punkteSpieler1 = 0;
    punkteSpieler2 = 0;
    starteNeueRunde();
    spielLaeuft = true;
  }
}

/* Erhöht ggf. die Geschwindigkeit */
void beschleunige() {
  /*
   * Zeit pro Schritt verringern, mit allen 6 Ballwechseln
   */
  if(ballwechsel % 6 == 0) {
    zeitProSchritt = startZeitProSchritt - ballwechsel / 2;
  }
}

void aktualisiereAnzeige() {
  /*
   * Setzen der Punkte
   */
  DigitShield.setDigit(1, punkteSpieler1/10);
  DigitShield.setDigit(2, punkteSpieler1%10);
  DigitShield.setDigit(3, punkteSpieler2/10);
  DigitShield.setDigit(4, punkteSpieler2%10);
  
  /*
   * Der Dezimalpunkt zeigt an, ob ein Spieler im Moment
   * den Knopf drücken darf
   */
  DigitShield.setDecimalPoint(2, spieler1Aktiv());
  DigitShield.setDecimalPoint(4, spieler2Aktiv());
  
  /*
   * Schickt die Daten an das Shield, so dass die Anzeige erscheint
   */
  DigitShield.isr(); 
}

/*
 * Gibt true zurück, wenn der Button von Spieler 1 gedrückt ist, anderenfalls false
 */
boolean spieler1ButtonGedrueckt() {
  return digitalRead(PIN_BUTTON_SPIELER_1);
}

/*
 * Gibt true zurück, wenn der Button von Spieler 2 gedrückt ist, anderenfalls false
 */
boolean spieler2ButtonGedrueckt() {
  return digitalRead(PIN_BUTTON_SPIELER_2);
}

/*
 * Gibt true zurück, wenn der Start-Button gedrückt ist, anderenfalls false
 */
boolean startButtonGedrueckt() {
  return digitalRead(PIN_BUTTON_START);
}

/*
 * Gibt true zurück, wenn der Laser in der Zone von Spieler 1 ist und Spieler 1 den 
 * Knopf drücken darf.
 */
boolean spieler1Aktiv() {
  return servoPosition <= ausPositionSpieler1 && servoPosition >= ausPositionSpieler1-breiteAktivZone;
}

/*
 * Gibt true zurück, wenn der Laser in der Zone von Spieler 2 ist und Spieler 2 den 
 * Knopf drücken darf.
 */
boolean spieler2Aktiv() {
  return servoPosition >= ausPositionSpieler2 && servoPosition <= ausPositionSpieler2+breiteAktivZone;
}

/* Resettet alle Werte für eine neue Runde */
void starteNeueRunde() {
  
  /* Fahre Servo auf die Mittelposition */
  servoPosition = (ausPositionSpieler1 + ausPositionSpieler2)/2;
  servo.write(servoPosition);
  
  /* Wähle eine zufällige Richtung */
  servoRichtung = random(2)*2-1;
  
  /* Setze alle Variablen für eine neue Runde */
  naechsterRundenStart = millis() + wartezeitVorRunde;
  spieler1Blockiert = false;
  spieler2Blockiert = false;
  zeitProSchritt = startZeitProSchritt;
  ballwechsel = 0;
}

/*
 * Kalibriere die Positionen der einzelnen Spieler
 */
void kalibriere() {
  if(! spieler1Kalibriert) {
    /*
     * Wenn Spieler 1 noch nicht kalibriert ist,
     * wähle die Grenze.
     */
    if(waehleGrenze()) {
      /*
       * Wenn die Grenze gewählt wurde, merken der Position
       * und dass wir den ersten Spieler kalibriert haben
       */
      Serial.println(servoPosition);
    
       
      ausPositionSpieler1 = servoPosition; 
      spieler1Kalibriert = true;
      delay(200);
    }
  } else if(! spieler2Kalibriert) {
    /*
     * Wenn Spieler 2 noch nicht kalibriert ist,
     * wähle die Grenze.
     */
    if(waehleGrenze()) {
      /*
       * Wenn die Grenze gewählt wurde, merken der Position
       * und dass wir den zweiten Spieler kalibriert haben
       */
      Serial.println(servoPosition);
      ausPositionSpieler2 = servoPosition; 
      spieler2Kalibriert = true;
      delay(200);
    }
  } else {
    /*
     * Falls Position von Spieler 1 rechts von der PositionSpieler 2
     * liegt (je weiter links, desto größer die Position),
     * vertauschen wir die beiden Positionen
     */
    if(ausPositionSpieler1 < ausPositionSpieler2) {
      int temp = ausPositionSpieler1;
      ausPositionSpieler1 = ausPositionSpieler2;
      ausPositionSpieler2 = temp; 
    }
    /* 
     * Spieler 1 und Spieler 2 sind fertig mit kalibrieren, damit sind 
     * wir fertig mit kalibrieren
     */
    kalibriert = true;
    
    /*
     * Starte eine neue Runde
     */
    starteNeueRunde();
  }
}

/*
 * Wähle eine Grenze bis Start gedrückt wurde. Mit den Knöpfen von Spieler 1 
 * und Spieler 2 lässt sich der Servo in beide Richtungen bewegen.
 */ 
boolean waehleGrenze() {
  /*
   * bestimme die Richtung, in die sich der Servo bewegt, abhängig
   * davon, welcher Knopf gedrückt ist.
   */
  if(spieler1ButtonGedrueckt()) {

    servoRichtung = 1;

  } else if(spieler2ButtonGedrueckt()) {

    servoRichtung = -1;

  } else {

    /*
     * Wenn kein Knopf gedrückt ist, bleibt der Servo stehen
     */
    servoRichtung = 0;
  } 

  /*
   * Bewege den Servo, wenn nötig 
   */
  bewegeServo();
  
  /*
   * Die Grenze ist ausgewählt, wenn der Startknopf gedrückt wurde
   */
  return startButtonGedrueckt();
}

/*
 * Bewegt den Servo, wenn es wieder Zeit dafür ist,
 * abhängig davon, ob die vergangene Zeit seit dem letzten Schritt
 * größer ist, als die Zeit, die zwischen den einzelnen Schritten liegt.
 */
void bewegeServo() {
  long aktuelleZeit = millis();
  /*
   * berechnen der Zeit, seit dem letzten Schritt
   */
  long zeitSeitLetztemSchritt = aktuelleZeit - letzterServoSchritt;
  
  /*
   * Wenn die Zeit seit dem letzten Schritt größer ist, als die Zeit, die 
   * pro Schritt verstreichen soll
   */
  if(zeitSeitLetztemSchritt > zeitProSchritt) {
    /*
     * Die Position des Servos um servoRichtung ändern
     */
    servoPosition += servoRichtung;
    
    /*
     * Begrenzen der Servoposition auf werte von 0 bis 180
     */
    if(servoPosition < 0) {
      servoPosition = 0;
    } else if (servoPosition > 180) {
      servoPosition = 180;
    }
    
    /*
     * Bewegen des Servos auf die neue Position
     */
    servo.write(servoPosition);
    
    /*
     * aktualisieren der Zeit, des letzten Schritts
     */
    letzterServoSchritt = aktuelleZeit;
  }
}


