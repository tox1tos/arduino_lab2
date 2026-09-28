const int RED_PIN = 13;
const int YELLOW_PIN = 12;
const int GREEN_PIN = 11;

const int PED_BUTTON_PIN = 2;
const int SERVICE_BUTTON_PIN = 3;


const unsigned long GREEN_TIME = 10000;       
const unsigned long YELLOW_TIME = 3000;       
const unsigned long RED_TIME = 10000;        

const unsigned long PED_EXTRA_TIME = 5000;    

const unsigned long WARNING_BLINK_TIME = 500; 
const unsigned long DEBOUNCE_TIME = 50;       



enum State {
  S_GREEN,
  S_YELLOW,
  S_RED,
  S_WARNING
};

State currentState = S_GREEN;

unsigned long stateStart = 0;
unsigned long stateDuration = 0;
bool pedRequest = false;
bool serviceMode = false;
unsigned long warningBlinkStart = 0;
bool warningLedState = false;

int pedButtonState = HIGH;
int lastPedReading = HIGH;
unsigned long pedDebounceStart = 0;

int serviceButtonState = HIGH;
int lastServiceReading = HIGH;
unsigned long serviceDebounceStart = 0;

const char* stateName(State state) {

  switch (state) {
    case S_GREEN:
      return "GREEN";

    case S_YELLOW:
      return "YELLOW";

    case S_RED:
      return "RED";

    case S_WARNING:
      return "WARNING";

    default:
      return "UNKNOWN";
  }
}


void setOutputsForState(State state) {
  digitalWrite(RED_PIN, LOW);
  digitalWrite(YELLOW_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);


  switch (state) {
    case S_GREEN:
      digitalWrite(GREEN_PIN, HIGH);
      break;
    case S_YELLOW:
      digitalWrite(YELLOW_PIN, HIGH);
      break;
    case S_RED:
      digitalWrite(RED_PIN, HIGH);
      break;
    case S_WARNING:
      break;
  }
}


void goToState(
  State newState,
  unsigned long duration,
  const char* reason
) {

  currentState = newState;

  stateStart = millis();
  stateDuration = duration;

  setOutputsForState(newState);

  if (newState == S_WARNING) {
    warningLedState = true;
    warningBlinkStart = millis();
    digitalWrite(YELLOW_PIN, HIGH);
  }

//логи для вывода
  Serial.print("[");
  Serial.print(millis());
  Serial.print(" ms] STATE -> ");

  Serial.print(stateName(newState));

  Serial.print(" | Reason: ");
  Serial.println(reason);
}

bool buttonPressed(
  int pin,
  int &stableState,
  int &lastReading,
  unsigned long &debounceStart
) {

  unsigned long now = millis();

  int reading = digitalRead(pin);

  if (reading != lastReading) {

    lastReading = reading;
    debounceStart = now;
  }


  if (now - debounceStart >= DEBOUNCE_TIME) {

    if (reading != stableState) {

      stableState = reading;

      if (stableState == LOW) {
        return true;
      }
    }
  }

  return false;
}

void readButtons() {
  bool servicePressed = buttonPressed(
    SERVICE_BUTTON_PIN,
    serviceButtonState,
    lastServiceReading,
    serviceDebounceStart
  );


  if (servicePressed) {

    serviceMode = !serviceMode;
    pedRequest = false;


    if (serviceMode) {
      goToState(
        S_WARNING,
        0,
        "Service mode ON"
      );
    }

    else {
      goToState(
        S_GREEN,
        GREEN_TIME,
        "Service mode OFF"
      );
    }
  }


  bool pedPressed = buttonPressed(
    PED_BUTTON_PIN,
    pedButtonState,
    lastPedReading,
    pedDebounceStart
  );

  if (
    pedPressed &&
    !serviceMode &&
    currentState == S_GREEN
  ) {

    pedRequest = true;

    Serial.print("[");
    Serial.print(millis());
    Serial.println(" ms] EVENT: Pedestrian request");
  }
}


void handleWarning() {
  unsigned long now = millis();
  if (
    now - warningBlinkStart >= WARNING_BLINK_TIME
  ) {

    warningBlinkStart = now;

    warningLedState = !warningLedState;


    if (warningLedState) {
      digitalWrite(YELLOW_PIN, HIGH);
    }

    else {
      digitalWrite(YELLOW_PIN, LOW);
    }
  }
}

void setup() {
  pinMode(RED_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);

  pinMode(PED_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SERVICE_BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(9600);
  goToState(
    S_GREEN,
    GREEN_TIME,
    "System start"
  );
}

void loop() {

  readButtons();

  switch (currentState) {
    case S_GREEN:
      if (
        millis() - stateStart >= stateDuration
      ) {

        goToState(
          S_YELLOW,
          YELLOW_TIME,
          "Green timer expired"
        );
      }
      break;

    case S_YELLOW:
      if (
        millis() - stateStart >= stateDuration
      ) {
        if (pedRequest) {

          pedRequest = false;


          goToState(
            S_RED,
            RED_TIME + PED_EXTRA_TIME,
            "Pedestrian request"
          );
        }


        else {
          goToState(
            S_RED,
            RED_TIME,
            "Yellow timer expired"
          );
        }
      }
      break;

    case S_RED:
      if (
        millis() - stateStart >= stateDuration
      ) {

        goToState(
          S_GREEN,
          GREEN_TIME,
          "Red timer expired"
        );
      }
      break;


//режим сервиса
    case S_WARNING:
      handleWarning();
      break;

    default:

      serviceMode = true;
      pedRequest = false;


      goToState(
        S_WARNING,
        0,
        "FSM error"
      );

      break;
  }
}
