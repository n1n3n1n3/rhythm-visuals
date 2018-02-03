#include <limits.h>

//Gives number of microseconds for corresponding duration
//Used to improve readability of configuration
const unsigned long MICROSECOND = 1;
const unsigned long MILLISECOND = 1000;
const unsigned long SECOND = 1000000;

const int NUM_SENSORS = 4;

//analog pin numbers for each sensor
const int SENSOR_PINS[NUM_SENSORS] = {0, 1, 2, 3};

/*SERIAL CONFIG*/

//print readings to arduino plotter
const boolean DEBUG = false;

//Serial communication Hz
const int BAUD_RATE = 115200;

//Delay in microseconds adter each line of debug messages
//Blocking (uses delay() function)
//Prevents overloading serial communications
const int PRINT_DELAY = 50 * MICROSECOND;

/*MIDI CONFIG*/

const boolean WITH_MIDI = true;
const int NOTES[NUM_SENSORS] = {60, 62, 64, 65};
const int MIDI_CHANNEL = 1;
const int BANK = 127;
const int PROGRAM = 0;

/*MOTOR CONFIG*/

const boolean WITH_MOTORS = false;
const int NUM_MOTORS = 2;

//digital pin numbers for each sensor
const int MOTOR_PINS[NUM_MOTORS] = {12, 24};

//Used as substitute for motors
const int LED_PIN = 13;

//index of MOTO_PIN to map for each sensor
//Needs to be in the range [-1, NUM_MOTORS].
//Uses LED_PIN instead of motor when -1
const int SENSOR_TO_MOTOR[NUM_SENSORS] = {0, 1, 0, 1};

//To limit duty cycle
unsigned const long MAX_MOTOR_PULSE_DURATION = 200 * MILLISECOND;

/*SENSOR CONFIG*/

//Maximum value returned by AnalogRead()
//Normally 1023 with arduino, but the operational amplifiers
//used in the sensor circuitry have a  maximum output voltage
//of 2V when powered at 3.3V
const int MAX_READING = 700;

//MIN_THRESHOLD is used when the baseline is very stable
const int MIN_THRESHOLD = 125;

//MAX_THRESHOLD is used when the baseline is very unstable
const int MAX_THRESHOLD = 200;

//Time between threshold traversal and rising() signal
//Allows for velocity measurment and ignoring very short jumps
unsigned const long NOTE_VELOCITY_DELAY = 1 * MILLISECOND;

//Delay in microseconds after sending rising() signal
//for which no more signals are sent for that sensor
unsigned const long NOTE_ON_DELAY = 65 * MILLISECOND;

//Delay in microseconds after sending falling() signal
//for which no more signals are sent for that sensor
unsigned const long NOTE_OFF_DELAY = 65 * MILLISECOND;

//Delay in microseconds between sustained() signals
//Also the delay between rising() and sustained()
unsigned const long SUSTAIN_DELAY = 100 * MILLISECOND;

//Delay in micro seconds between baseline samples
unsigned const long BASELINE_SAMPLE_DELAY = 0.5 * MILLISECOND;

//number of microseconds after jump during which baseline update is paused
unsigned const long BASELINE_BLOWBACK_DELAY = 40 * MILLISECOND;

//TODO: change constant to timing notation
//amount of baseline samples that we average baseline over
//Multiply with BASELINE_SAMPLE_DELAY to get baseline update duration.
const int BASELINE_BUFFER_SIZE = 1000;

//TODO: move division to the code
//number of samples removed from baseline buffer when jump is over
//This is used to prevent rising edge portion of signal
//that is below threshold from weighing in on baseline.
//Making too large would prevent baseline update while fast-tapping.
//Multiply with BASELINE_SAMPLE_DELAY  to get the rise time to reach the threshold.
const int RETRO_JUMP_BLOWBACK_SAMPLES = (0.5 * MILLISECOND) / BASELINE_SAMPLE_DELAY;

//TODO: move division to the code
//After this amount of sustains
//the baseline is reset to the last sensor reading
const int MAX_CONSECUTIVE_SUSTAINS = (10 * SECOND) / SUSTAIN_DELAY;

//*GLOBAL VARIABLES*

//current baseline for each pin
int baseline[NUM_SENSORS];

//current threshold
int jumpThreshold[NUM_SENSORS];

void setup() {
  if (DEBUG || WITH_MIDI) {
    Serial.begin(BAUD_RATE);
  }
  for (int sensor = 0; sensor < NUM_SENSORS; sensor++) {
    baseline[sensor] = analogRead(SENSOR_PINS[sensor]);
    jumpThreshold[sensor] = (MIN_THRESHOLD + MAX_THRESHOLD) / 2;
  }

  //write LOW to motors even if WITH_MOTORS is false
  //just to be sure they stay off if they are still plugged in
  if (NUM_MOTORS > 0) {
    pinMode(LED_PIN, OUTPUT);

    for (int motor = 0; motor < NUM_MOTORS; motor++) {
      pinMode(MOTOR_PINS[motor], OUTPUT);
      digitalWrite(MOTOR_PINS[motor], LOW);
    }
  }

  //wait to ensure Midi mapper has had time to detect midi input
  delay(5000);

  //Set midi soundfont bank
  usbMIDI.sendControlChange(0, BANK, MIDI_CHANNEL);
  usbMIDI.send_now();

  // MIDI Controllers should discard incoming MIDI messages.
  while (usbMIDI.read()) {}

  //Set midi instrument
  usbMIDI.sendProgramChange(PROGRAM, MIDI_CHANNEL);
  usbMIDI.send_now();

  // MIDI Controllers should discard incoming MIDI messages.
  while (usbMIDI.read()) {}
}

void loop() {
  //*STATIC VARIABLES*

  //set to true after rising() signal
  static bool justJumped[NUM_SENSORS];

  //Sensor values while not jumping
  static int baselineBuffer[NUM_SENSORS][BASELINE_BUFFER_SIZE];
  static int baselineBufferIndex[NUM_SENSORS];

  //number of sustained() signals (minus two) sent for current jump
  //Also incremented when threshold is first traversed 
  //and when rising() signal is sent thereafter
  static int sustainCount[NUM_SENSORS];

  //used to delay baseline calculation after coming out of jump and between samples
  static unsigned long toWaitBeforeBaseline[NUM_SENSORS];

  //used to delay midi signals from one another
  static unsigned long toWaitBeforeRising[NUM_SENSORS];
  static unsigned long toWaitBeforeFalling[NUM_SENSORS];
  static unsigned long toWaitBeforeSustaining[NUM_SENSORS];

  //used to calculate time difference in microseconds while waiting
  //lastRisingTime is used for both toWaitBeforeRising and toWaitBeforeFalling
  //since these never overlap
  static unsigned long lastRisingTime[NUM_SENSORS];
  static unsigned long lastSustainingTime[NUM_SENSORS];
  static unsigned long lastBaselineTime[NUM_SENSORS];

  //for debug
  int toPrint[NUM_SENSORS];
  memset(toPrint, 0, sizeof(toPrint));

  for (int currentSensor = 0; currentSensor < NUM_SENSORS; currentSensor++) {
    int sensorReading = analogRead(SENSOR_PINS[currentSensor]);
    int distanceAboveBaseline = max(0, sensorReading - baseline[currentSensor]);

    if (DEBUG) {
      toPrint[currentSensor] = sensorReading;
    }

    //JUMPING
    if (distanceAboveBaseline >= jumpThreshold[currentSensor]) {
      //VELOCITY OFFSET
      if (sustainCount[currentSensor] == 0) {
        //WAIT
        if (toWaitBeforeRising[currentSensor] > 0) {
          updateRemainingTime(toWaitBeforeRising[currentSensor], lastRisingTime[currentSensor]);
        }
        //TRIGGER DELAY
        else {
          lastRisingTime[currentSensor] = micros();
          toWaitBeforeRising[currentSensor] = NOTE_VELOCITY_DELAY;
          sustainCount[currentSensor]++;
        }
      }
      //RISING
      else if (sustainCount[currentSensor] == 1) {
        //WAIT
        if (toWaitBeforeRising[currentSensor] > 0) {
          updateRemainingTime(toWaitBeforeRising[currentSensor], lastRisingTime[currentSensor]);
        }
        //SIGNAL
        else {
          rising(currentSensor, distanceAboveBaseline);

          lastRisingTime[currentSensor] = micros();
          toWaitBeforeFalling[currentSensor] = NOTE_ON_DELAY;

          lastSustainingTime[currentSensor] = micros();
          toWaitBeforeSustaining[currentSensor] = SUSTAIN_DELAY;

          justJumped[currentSensor] = true;
          sustainCount[currentSensor]++;
        }
      }
      //SUSTAINING
      else {
        //RESET
        if (sustainCount[currentSensor] > MAX_CONSECUTIVE_SUSTAINS) {
          baseline[currentSensor] = sensorReading;

          //reset counters
          baselineBufferIndex[currentSensor] = 0;
          sustainCount[currentSensor] = 0;
        }
        //WAIT
        else if (toWaitBeforeSustaining[currentSensor] > 0) {
          updateRemainingTime(toWaitBeforeSustaining[currentSensor], lastSustainingTime[currentSensor]);
        }
        //SIGNAL
        else {
          sustained(currentSensor, distanceAboveBaseline, NOTE_VELOCITY_DELAY + ((sustainCount[currentSensor] - 1) * SUSTAIN_DELAY));

          lastSustainingTime[currentSensor] = micros();
          toWaitBeforeSustaining[currentSensor] = SUSTAIN_DELAY;
          sustainCount[currentSensor]++;
        }
      }
    }
    //NOT JUMPING
    else {
      //FALLING
      if (justJumped[currentSensor]) {
        //WAIT
        if (toWaitBeforeFalling[currentSensor]) {
          updateRemainingTime(toWaitBeforeFalling[currentSensor], lastRisingTime[currentSensor]);
        }
        //SIGNAL
        else {
          falling(currentSensor);

          //wait before sending more midi signals
          //debounces falling edge
          lastRisingTime[currentSensor] = micros();
          toWaitBeforeRising[currentSensor] = NOTE_OFF_DELAY;

          //wait before buffering baseline
          //this is to ignore the sensor "blowback" (erratic readings after jumps)
          //and remove falling edge portion of signal that is below threshold
          lastBaselineTime[currentSensor] = micros();
          toWaitBeforeBaseline[currentSensor] = BASELINE_BLOWBACK_DELAY;

          justJumped[currentSensor] = false;

          //backtrack baseline count to remove jump start
          //(might not do anything if we just updated baseline)
          baselineBufferIndex[currentSensor] = max( 0, baselineBufferIndex[currentSensor] - RETRO_JUMP_BLOWBACK_SAMPLES);

          //reset jump counter
          sustainCount[currentSensor] = 0;
        }
      }
      //BASELINING
      else {
        //reset jump counter
        sustainCount[currentSensor] = 0;

        //RESET
        if (baselineBufferIndex[currentSensor] > (BASELINE_BUFFER_SIZE - 1)) {
          jumpThreshold[currentSensor] = updateThreshold(baselineBuffer[currentSensor], baseline[currentSensor], jumpThreshold[currentSensor]);
          baseline[currentSensor] = bufferAverage(baselineBuffer[currentSensor], BASELINE_BUFFER_SIZE);

          //reset counter
          baselineBufferIndex[currentSensor] = 0;
        }
        //WAIT
        else if (toWaitBeforeBaseline[currentSensor] > 0) {
          updateRemainingTime(toWaitBeforeBaseline[currentSensor], lastBaselineTime[currentSensor]);
        }
        //SAMPLE
        else {
          baselineBuffer[currentSensor][baselineBufferIndex[currentSensor]] = sensorReading;
          baselineBufferIndex[currentSensor]++;

          //reset timer
          lastBaselineTime[currentSensor] = micros();
          toWaitBeforeBaseline[currentSensor] = BASELINE_SAMPLE_DELAY;
        }
      }
    }
  }
  if (DEBUG) {
    printResults(toPrint, sizeof(toPrint) / sizeof(int));
  }
}
//*HELPERS*

int bufferAverage(int * a, int aSize) {
  unsigned long sum = 0;
  int i;
  for (i = 0; i < aSize; i++) {
    //makes sure we dont bust when filling up sum
    if (sum < (ULONG_MAX - a[i])) {
      sum += a[i];
    }
    else {
      Serial.println("WARNING: Exceeded ULONG_MAX while running bufferAverage(). Check your parameters to ensure buffers aren't too large.");
      delay(1000);
      break;
    }
  }
  return (int) (sum / i);
}

int varianceFromTarget(int * a, int aSize, int target) {
  unsigned long sum = 0;
  int i;
  for (i = 0; i < aSize; i++) {
    //makes sure we dont bust when filling up sum
    int toAdd = pow( (a[i] - target), 2);
    if (sum < ULONG_MAX - toAdd) {
      sum += toAdd;
    }
    else {
      Serial.println("WARNING: Exceeded ULONG_MAX while running varianceFromTarget(). Check your parameters to ensure buffers aren't too large.");
      delay(1000);
      break;
    }
  }

  return (int) (sum / i);
}

//updates time left to wait and given last time that are both passed by reference
void updateRemainingTime(unsigned long (&left), unsigned long (&last)) {
  unsigned long thisTime = micros();
  unsigned long deltaTime = thisTime - last;

  if (deltaTime < left) {
    left -= deltaTime;
  } else {
    left = 0;
  }

  last = thisTime;
}

//Single use function to improve readability
int updateThreshold(int (&baselineBuff)[BASELINE_BUFFER_SIZE], int oldBaseline, int oldThreshold) {

  int varianceFromBaseline = varianceFromTarget(baselineBuff, BASELINE_BUFFER_SIZE, oldBaseline);
  int newThreshold = constrain(varianceFromBaseline, MIN_THRESHOLD, MAX_THRESHOLD);

  int deltaThreshold = newThreshold - oldThreshold;
  if (deltaThreshold < 0) {
    //split the difference to slow down threshold becoming more sensitive
    newThreshold = constrain(oldThreshold + ((deltaThreshold) / 4), MIN_THRESHOLD, MAX_THRESHOLD);
  }

  return newThreshold;
}


//print results for all sensors in Arduino Plotter format
//Note that running the debug slows down the rest of the script (requires delay to avoid overloading serial)
//so you'll have to compensate for the slowdown when setting parameters
void printResults(int toPrint[], int printSize) {
  for (int i = 0; i < printSize; i++) {
    Serial.print("0");
    Serial.print(" ");
    Serial.print(toPrint[i]);
    Serial.print(" ");
    Serial.print(baseline[i]);
    Serial.print(" ");
    Serial.print(baseline[i] + jumpThreshold[i]);
    Serial.print(" ");
  }
  Serial.print(MAX_READING);
  Serial.println();
  delayMicroseconds(PRINT_DELAY);
}

int sensorToMotor(int sensorIndex) {
  if (SENSOR_TO_MOTOR[sensorIndex] == -1) {
    //Turn on LED instead of motor
    return LED_PIN;
  }
  else {
    return MOTOR_PINS[SENSOR_TO_MOTOR[sensorIndex]];
  }
}

void rising(int sensor, int velocity) {
  if (WITH_MOTORS) {
    digitalWrite(sensorToMotor(sensor), HIGH);
  }
  if (WITH_MIDI) {
    int maxVelocity = MAX_READING - baseline[sensor];
    int constrainedVelocity = constrain(velocity, jumpThreshold[sensor], maxVelocity);
    int scaledVelocity =  map(constrainedVelocity, jumpThreshold[sensor], maxVelocity, 64, 127);

    usbMIDI.sendNoteOn(NOTES[sensor], scaledVelocity, MIDI_CHANNEL);
    usbMIDI.send_now();

    // MIDI Controllers should discard incoming MIDI messages.
    while (usbMIDI.read()) {}
  }
}

void falling(int sensor) {
  if (WITH_MOTORS) {
    digitalWrite(sensorToMotor(sensor), LOW);
  }

  if (WITH_MIDI) {
    usbMIDI.sendNoteOff(NOTES[sensor], 0, MIDI_CHANNEL);
    usbMIDI.send_now();

    // MIDI Controllers should discard incoming MIDI messages.
    while (usbMIDI.read()) {}
  }
}

void sustained(int sensor, int velocity, unsigned long duration) {
  if (WITH_MOTORS) {
    if (duration >= MAX_MOTOR_PULSE_DURATION) {
      digitalWrite(sensorToMotor(sensor), LOW);
    }
  }
  if (WITH_MIDI) {
    //    usbMIDI.sendPolyPressure(NOTES[sensor], map(constrain(velocity, jumpThreshold[sensor], 512), jumpThreshold[sensor], 512, 64, 127), MIDI_CHANNEL);
    //    usbMIDI.send_now();
    //
    //    // MIDI Controllers should discard incoming MIDI messages.
    //    while (usbMIDI.read()) {}
  }
}

