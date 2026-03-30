#include <AccelStepper.h>

const int stepPin = 2;    //green
const int dirPin  = 15;   //yellow

const int stepPin2 = 23;  //green
const int dirPin2  = 22;  //yellow

const int analog_pin = 4;
const int tx_pin = 5;

AccelStepper stepper(1, stepPin, dirPin);
AccelStepper stepper2(1, stepPin2, dirPin2);

// Sensor state
long capacitance_value = 0;
long sensor_sum = 0;
int sensor_count = 0;
const int N_samples = 100;   // average over this many samples

// Timing
unsigned long lastPrintTime = 0;
const unsigned long printInterval = 200; // ms

void setup()
{
  Serial.begin(9600);
  delay(1000);
  Serial.println("beginning");

  pinMode(tx_pin, OUTPUT);
  digitalWrite(tx_pin, LOW);

  stepper.setMaxSpeed(1000);
  stepper2.setMaxSpeed(1000);

  stepper.setSpeed(0);
  stepper2.setSpeed(0);
}

void loop()
{
  // Keep motors stepping continuously
  stepper.runSpeed();
  stepper2.runSpeed();

  // Do sensor sample per loop pass
  updateCapSensor();

  // Decide motion based on latest averaged value
  if (capacitance_value > 4000) {
    stepper.setSpeed(50);
    stepper2.setSpeed(50);
  } else {
    stepper.setSpeed(0);
    stepper2.setSpeed(0);
  }
}

void updateCapSensor()
{
  int read_high;
  int read_low;
  int diff;

  digitalWrite(tx_pin, HIGH);
  delayMicroseconds(100);
  read_high = analogRead(analog_pin);

  digitalWrite(tx_pin, LOW);
  delayMicroseconds(100);
  read_low = analogRead(analog_pin);

  diff = read_high - read_low;

  sensor_sum += diff;
  sensor_count++;

  if (sensor_count >= N_samples) {
    capacitance_value = sensor_sum / N_samples;
    sensor_sum = 0;
    sensor_count = 0;
  }
}