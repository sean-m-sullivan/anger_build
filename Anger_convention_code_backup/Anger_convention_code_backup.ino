#define REMOTE_PIN A0

#define IN1 5
#define IN2 6

// =========================
// SETTINGS
// =========================

// number of full forward/reverse cycles
int totalLoops = 7;

// motor runtime per direction (milliseconds)
unsigned long runTime = 300;
// extra reverse runtime compensation

unsigned long reverseOffset = 48;

// pause between direction changes (milliseconds)
unsigned long pauseTime = 2000;

// PWM speed 0-255
int motorSpeed = 255;

// =========================

bool running = false;
bool lastState = HIGH;
bool stopRequested = false;

unsigned long phaseStart = 0;
unsigned long cycleStart = 0;

// auto-demo settings
unsigned long idleTimeout = 300000UL; // 5 minutes
unsigned long lastActivityTime = 0;
unsigned long tantrumStartTime = 0;

// 300000UL   // 5 min
// 600000UL   // 10 min
// 900000UL   // 15 min

int loopCount = 0;
bool directionForward = true;

enum MotorState {
  RUNNING,
  PAUSED
};

MotorState motorState = RUNNING;

void stopMotor() {

  analogWrite(IN1, 0);
  analogWrite(IN2, 0);

  digitalWrite(LED_BUILTIN, LOW);

  Serial.print("[");
  Serial.print(millis());
  Serial.println(" ms] MOTOR STOPPED");
}

void runForward() {

  analogWrite(IN1, motorSpeed);
  analogWrite(IN2, 0);

  digitalWrite(LED_BUILTIN, HIGH);

  Serial.print("[");
  Serial.print(millis());
  Serial.print(" ms] ");

  Serial.print("FORWARD  PWM=");
  Serial.print(motorSpeed);

  Serial.print("  Runtime=");
  Serial.print(runTime);
  Serial.println(" ms");
}

void runReverse() {

  analogWrite(IN1, 0);
  analogWrite(IN2, motorSpeed);

  digitalWrite(LED_BUILTIN, HIGH);

  Serial.print("[");
  Serial.print(millis());
  Serial.print(" ms] ");

  Serial.print("REVERSE  PWM=");
  Serial.print(motorSpeed);

  Serial.print("  Runtime=");
  Serial.print(runTime + reverseOffset);
  Serial.println(" ms");
}

void setup() {

  pinMode(REMOTE_PIN, INPUT_PULLUP);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(9600);

  stopMotor();

  Serial.println("SYSTEM READY");
  lastActivityTime = millis();
}

void loop() {

  bool currentState = digitalRead(REMOTE_PIN);

  // =========================
  // DEBUG REMOTE STATE
  // =========================
  static bool lastPrintedState = HIGH;

  if (currentState != lastPrintedState) {

    Serial.print("REMOTE: ");

    if (currentState == LOW) {
      Serial.println("PRESSED");
    } else {
      Serial.println("RELEASED");
    }

    lastPrintedState = currentState;
  }

  // =========================
  // BUTTON PRESSED
  // =========================
  if (lastState == HIGH && currentState == LOW) {

    // START
    if (!running) {
      
      Serial.println("========== START ==========");

      running = true;

      loopCount = 0;

      directionForward = true;

      motorState = RUNNING;

      phaseStart = millis();
      cycleStart = millis();

      lastActivityTime = millis();

      runForward();

    }

    // STOP EARLY
    else {

      Serial.println("STOP REQUESTED - RETURNING HOME");

      stopRequested = true;
    }

    // debounce
    delay(250);
  }

  lastState = currentState;
// =========================
// AUTO TANTRUM
// =========================

if (!running &&
    millis() - lastActivityTime >= idleTimeout) {

  Serial.println("AUTO TANTRUM");

  running = true;

  loopCount = 0;

  directionForward = true;

  motorState = RUNNING;

  phaseStart = millis();
  cycleStart = millis();

  lastActivityTime = millis();

  runForward();
}

  // =========================
  // MOTOR TEST LOOP
  // =========================
  if (running) {

    // =========================
    // RUNNING STATE
    // =========================
    if (motorState == RUNNING) {

      // determine runtime for current direction
      unsigned long currentRunTime;

      if (directionForward) {

        currentRunTime = runTime;

      } else {

        currentRunTime = runTime + reverseOffset;
      }

      // run motor for configured time
      if (millis() - phaseStart >= currentRunTime) {

        stopMotor();

        motorState = PAUSED;

        phaseStart = millis();

        Serial.print("[");
        Serial.print(millis());
        Serial.println(" ms] PAUSE");
      }
    }

    // =========================
    // PAUSED STATE
    // =========================
    else if (motorState == PAUSED) {

      // wait pause duration
      if (millis() - phaseStart >= pauseTime) {

        // toggle direction
        directionForward = !directionForward;

        // =========================
        // SWITCHING TO FORWARD
        // =========================
        if (directionForward) {

          loopCount++;

          unsigned long cycleTime = millis() - cycleStart;

          Serial.println("--------------------------------");

          Serial.print("COMPLETED LOOP: ");
          Serial.println(loopCount);

          Serial.print("TOTAL CYCLE TIME: ");
          Serial.print(cycleTime);
          Serial.println(" ms");

          Serial.println("--------------------------------");

          cycleStart = millis();

          // stop BEFORE starting another cycle
          if (loopCount >= totalLoops || stopRequested) {

            Serial.println("RETURNED HOME");

          Serial.println("================================");
          Serial.println("AUTO STOP");
          Serial.print("TOTAL LOOPS COMPLETED: ");
          Serial.println(loopCount);
          Serial.print("TOTAL RUN TIME: ");
          Serial.print(millis() / 1000.0);
          Serial.println(" sec");
          Serial.println("================================");

            running = false;

            stopRequested = false;

            lastActivityTime = millis();

            stopMotor();

            return;
          }

          runForward();
        }

        // =========================
        // SWITCHING TO REVERSE
        // =========================
        else {

          runReverse();
        }

        motorState = RUNNING;

        phaseStart = millis();
      }
    }
  }
}