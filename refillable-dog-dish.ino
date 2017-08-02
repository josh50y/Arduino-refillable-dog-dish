// These values help bring the readings from the water in evenely and throw out errant readings.
int meter1[10];
int meter2[10];
byte sampleCount = 0;
byte currSample = 0;

byte readingLow = 0; // The reading on the LOWER sensor
byte readingHigh = 0; // The reading on the HIGHER sensor

byte blinkPattern = 0; // Number of blinks.
byte blinkPoint = 0;

// The pins where items are connected
#define PIN_SENSE_LOW 5    // The LOW sense wire (bottom of dish)
#define PIN_SENSE_HIGH 9   // The HIGH sense wire (top of dish)
#define PIN_WATER 12       // Turn on the water flow 
#define PIN_BUZZER 6       // Piezo buzzer for alerts
#define PIN_SENSE_POWER 8  // Turn on the power for the sensors to prevent corrosion

void setup() {                
  // Pin 13 has an LED connected on most Arduino boards:
  pinMode(13, OUTPUT);     
  pinMode(PIN_SENSE_LOW, INPUT);
  pinMode(PIN_SENSE_HIGH, INPUT);
  pinMode(PIN_WATER, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_SENSE_POWER, OUTPUT);
  
  // Make sure these things are off.
  digitalWrite(PIN_WATER, LOW);
  digitalWrite(PIN_SENSE_POWER, LOW);
  
  // Startup sound
  buzz(230);
  buzz(270);
  buzz(320);
  
  // Open the serial connection for debugging
  Serial.begin(9600);
}

void loop() {

  long timerA = 0;
  long timerB = 0;
  
  // Main logic loop.
  if (sampleCount < 10)   // Not enough readings yet. 10 readings are needed as a baseline.
  {
   updateSensors(); 
  }
  else
  {
    // LOGIC LOOP A
    blinkPattern = 60;
    Serial.println("A. Waiting for No Low");
    waitForNoLow();
    
    // At this point, the LOW sensor is not activated.
    // Try for another 30 seconds to see if it will come back.
    blinkPattern = 10;

    Serial.println("B. No Low Detected. Confirming for 30 seconds.");
    timerA = millis();
    do {
      delay(500);
      updateSensors();
      if (millis() - timerA > 30000 || millis() < timerA) break;  
    } while (readingLow == 0 && readingHigh == 0);
    
    // Are we still reading no low?
    if (readingLow == 0)
    {
      // Alert that we have entered this state via buzzer.
      buzz(230);
      delay(200);
      buzz(230);

      Serial.println("C. No Low confirmed. Evaporating for 10 minutes.");
      for (int i = 0; i < 1800; i++) 
      {
        doBlinks();
        delay(1000);      
      }
      
      // Turn on the water.
      blinkPattern = 3; 
      
      Serial.println("WATER: TURNING ON");
      buzz(270);
      buzz(230);
      
      digitalWrite(PIN_WATER, HIGH);

      // Start timer B.
      timerB = millis();
      
      // Monitor the water levels.
      Serial.println("D. Monitoring water levels...");
      do {
         delay(20);
         updateSensors();
        if (millis() - timerB > 35000 || millis() < timerB) 
        {
          Serial.println("Water should be full by now. Detection failure.");
          
          // Enter the error state.
          digitalWrite(PIN_WATER, LOW);
          do {
            alarm(5);
          } while (true); // Infinite loop, requires user reset.
          
          break;  
        }
      } while (readingHigh == 0);
      
      Serial.println("E. Fill level reached.");
      Serial.println("WATER: TURNING OFF");
      digitalWrite(PIN_WATER, LOW);

      buzz(230);
      buzz(270);
  
      // At this point the loop begins again.
    } else {
      Serial.println("False positive. The low water is back.");
    }
  }

  delay(20);
}

// This function waits for the dish to be empty.
void waitForNoLow()
{
    do {
      delay(10000); // Check once every 10 seconds.
      updateSensors();
    } while (readingLow == 1 || readingHigh == 1);
}

void doBlinks()
{
   if (blinkPoint == 0)
   {
     digitalWrite(13, HIGH);
     delay(10);
     digitalWrite(13, LOW);
   }
   
   blinkPoint++;
   if (blinkPoint > blinkPattern) blinkPoint = 0;
}   

void buzz(long nFreq)
{
  for (long i = 0; i < 1000; i++ )
  {
    // 1 / 2048Hz = 488uS, or 244uS high and 244uS low to create 50% duty cycle
    digitalWrite(PIN_BUZZER, HIGH);
    delayMicroseconds(nFreq);
    digitalWrite(PIN_BUZZER, LOW);
    delayMicroseconds(nFreq);
  }
}

void alarm(byte nCount)
{
  for (byte i = 0; i < nCount; i++)
  {
    buzz(230);
    buzz(270);
  } 
}

void updateSensors()
{
  // Take a sample
  digitalWrite(PIN_SENSE_POWER, HIGH);
  delay(1);
  
  byte m1 = digitalRead(PIN_SENSE_LOW);
  byte m2 = digitalRead(PIN_SENSE_HIGH);
  
  // Turn this off to not cause corrosion
  digitalWrite(PIN_SENSE_POWER, LOW);
  
  meter1[currSample] = m1;
  meter2[currSample] = m2;
  currSample++;
  if (currSample > 9) currSample = 0;
  if (sampleCount < 10)
  {
     // First 10 samples are not counted
     sampleCount++; 
  } else {
     
     // All samples have to be the same to change the reading.
     float t1 = 0;
     float t2 = 0;
     for (byte i = 0; i < 10; i++)
     {
        t1 = t1 + meter1[i]; 
        t2 = t2 + meter2[i]; 
     }
     
     t1 = t1 / 10;
     t2 = t2 / 10;
     
     if (t1 == 0 || t1 == 1) { // All samples are the same
       readingLow = t1;
     }

     if (t2 == 0 || t2 == 1) { // All samples are the same
       readingHigh = t2;
     }
  } 
  
  doBlinks();
}

