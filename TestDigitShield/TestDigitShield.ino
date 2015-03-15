#include <DigitShield.h>

void setup() {
  DigitShield.begin();
}

void loop() { 
  int nummer = (millis()/1000)%10;
    
  DigitShield.setDigit(1, nummer);
  DigitShield.setDigit(2, nummer);
  DigitShield.setDigit(3, nummer);
  DigitShield.setDigit(4, nummer);
  DigitShield.setDecimalPoint(1, nummer % 2);
  DigitShield.setDecimalPoint(2, nummer % 2);
  DigitShield.setDecimalPoint(3, nummer % 2);
  DigitShield.setDecimalPoint(4, nummer % 2);

  DigitShield.isr();  
}

