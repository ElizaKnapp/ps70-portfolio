#include <AccelStepper.h>

// NOTE: stepper1 uses negative moves to reverse

// ---------- Motor pins ----------
// left wheel
const int stepPin1 = 2;
const int dirPin1  = 19;

// right wheel
const int stepPin2 = 23;
const int dirPin2  = 22;

// ---------- Hall sensor pins ----------
const int hallPins[5] = {25, 33, 21, 35, 34};

const int stopPin  = hallPins[0];
const int startPin = hallPins[1];

const int fiveSecPin   = hallPins[2];
const int tenSecPin    = hallPins[3];
const int twentySecPin = hallPins[4];

// ---------- Adjustable variables ----------
const int motorSpeed = 800;
const int motorAcceleration = 500;

const int numReadings = 500;

// Movement step sizes
const long move1 = 500;
const long move2 = 3000;
const long move3 = 4000;

// Default timer value
unsigned long selectedWaitTime = 5000;

// ---------- Motors ----------
AccelStepper stepper1(AccelStepper::DRIVER, stepPin1, dirPin1);
AccelStepper stepper2(AccelStepper::DRIVER, stepPin2, dirPin2);

// ---------- State machine ----------
enum CarState {
  IDLE,
  REVERSING,
  WAITING,
  RETURNING
};

enum MovementState {
  MOVE_1,
  MOVE_2,
  MOVE_3,
  DONE
};

CarState state = IDLE;
MovementState movementState = DONE;

unsigned long waitStartTime = 0;

// Prevents retriggering while the same magnet placement is still active
bool startArmed = true;

// ---------- Non-blocking hall sensor averaging ----------
int hallStates[5] = {LOW, LOW, LOW, LOW, LOW};
int hallSums[5] = {0, 0, 0, 0, 0};
int hallSampleCount = 0;

void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println("beginning");

  // Set up hall sensor pins
  for (int i = 0; i < 5; i++) {
    if (hallPins[i] == 34 || hallPins[i] == 35) {
      pinMode(hallPins[i], INPUT);  // ESP32 pins 34/35 do not support internal pullups
    } else {
      pinMode(hallPins[i], INPUT_PULLUP);
    }
  }

  // Set up motor speeds and accelerations
  stepper1.setMaxSpeed(motorSpeed);
  stepper1.setAcceleration(motorAcceleration);

  stepper2.setMaxSpeed(motorSpeed);
  stepper2.setAcceleration(motorAcceleration);
}

void loop() {
  // Keep motors moving smoothly
  stepper1.run();
  stepper2.run();

  // Update hall sensors
  updateHallSensorsNonBlocking();

  // Update selected wait time
  readTimerPins();

  int stopReading = hallStates[0];
  int startReading = hallStates[1];

  // Stop mode overrides everything
  if (stopReading == HIGH) {
    stopMotorsNow();
    state = IDLE;
    movementState = DONE;
    startArmed = false;
    return;
  }

  // Only re-arm when idle AND the start magnet has been removed
  if (state == IDLE && startReading == LOW) {
    startArmed = true;
  }

  // Start only once per magnet placement
  if (state == IDLE && startReading == HIGH && startArmed) {
    startArmed = false;
    startReverse();
  }

  updateMotionState();
}

void updateHallSensorsNonBlocking() {
  for (int i = 0; i < 5; i++) {
    hallSums[i] += digitalRead(hallPins[i]);
  }

  hallSampleCount++;

  if (hallSampleCount >= numReadings) {
    for (int i = 0; i < 5; i++) {
      if (hallSums[i] >= (numReadings / 2 + 1)) {
        hallStates[i] = HIGH;
      } else {
        hallStates[i] = LOW;
      }

      hallSums[i] = 0;
    }

    hallSampleCount = 0;
  }
}

void readTimerPins() {
  if (hallStates[2] == HIGH) {
    selectedWaitTime = 5000;
  }

  if (hallStates[3] == HIGH) {
    selectedWaitTime = 10000;
  }

  if (hallStates[4] == HIGH) {
    selectedWaitTime = 20000;
  }
}

void startReverse() {
  Serial.println("Starting reverse");

  movementState = MOVE_1;
  state = REVERSING;
  // Reverse move 1: both motors
  stepper1.move(-move1);
  stepper2.move(move1);
}

void startReturnSequence() {
  Serial.println("Starting return sequence");

  movementState = MOVE_1;
  state = RETURNING;

  // Return move 1: exact opposite of reverse move 3
  stepper1.move(move3);
  stepper2.move(-move3);
}


void updateMotionState() {
  if (state == REVERSING) {
    updateReverseSequence();
  }

  else if (state == WAITING) {
    if (millis() - waitStartTime >= selectedWaitTime) {
      startReturnSequence();
    }
  }

  else if (state == RETURNING) {
    updateReturnSequence();
  }
}

void updateReverseSequence() {
  if (movementState == MOVE_1) {
    if (bothMotorsDone()) {
      Serial.println("Reverse move 1 complete");

      // Reverse move 2: only motor 1
      stepper1.move(-move2);

      movementState = MOVE_2;
    }
  }

  else if (movementState == MOVE_2) {
    if (stepper1.distanceToGo() == 0) {
      Serial.println("Reverse move 2 complete");

      // Reverse move 3: both motors
      stepper1.move(-move3);
      stepper2.move(move3);

      movementState = MOVE_3;
    }
  }

  else if (movementState == MOVE_3) {
    if (bothMotorsDone()) {
      Serial.println("Finished reversing, waiting");

      movementState = DONE;
      waitStartTime = millis();
      state = WAITING;
    }
  }
}

void updateReturnSequence() {
  if (movementState == MOVE_1) {
    if (bothMotorsDone()) {
      Serial.println("Return move 1 complete");

      // Return move 2: exact opposite of reverse move 2
      stepper1.move(move2);

      movementState = MOVE_2;
    }
  }

  else if (movementState == MOVE_2) {
    if (stepper1.distanceToGo() == 0) {
      Serial.println("Return move 2 complete");

      // Return move 3: exact opposite of reverse move 1
      stepper1.move(move1);
      stepper2.move(-move1);

      movementState = MOVE_3;
    }
  }

  else if (movementState == MOVE_3) {
    if (bothMotorsDone()) {
      Serial.println("Returned to start");

      movementState = DONE;
      state = IDLE;
      startArmed = false;
    }
  }
}

bool bothMotorsDone() {
  return stepper1.distanceToGo() == 0 && stepper2.distanceToGo() == 0;
}

void stopMotorsNow() {
  stepper1.setCurrentPosition(stepper1.currentPosition());
  stepper2.setCurrentPosition(stepper2.currentPosition());
}