const byte RED_PIN = 13;
const byte YELLOW_PIN = 12;
const byte GREEN_PIN = 11;
const byte PED_BUTTON = 2;
const byte SERVICE_BUTTON = 3;

const unsigned long GREEN_TIME = 10000;
const unsigned long YELLOW_TIME = 3000;
const unsigned long RED_TIME = 10000;
const unsigned long PED_EXTRA = 5000;
const unsigned long BLINK_TIME = 500;

enum State {
  S_GREEN,
  S_YELLOW,
  S_RED,
  S_WARNING,
  NUM_STATES
};

enum Event {
  E_TIMER,
  E_PED,
  E_SERVICE,
  NUM_EVENTS
};

State currentState = S_GREEN;

unsigned long stateStart;
unsigned long stateDuration;

bool pedRequest = false;

unsigned long blinkStart;
bool yellowState = false;

bool lastPed = HIGH;
bool lastService = HIGH;

typedef void (*Handler)();

Handler table[NUM_STATES][NUM_EVENTS];

const char* stateName(State state) {
  switch (state) {
    case S_GREEN: return "GREEN";
    case S_YELLOW: return "YELLOW";
    case S_RED: return "RED";
    case S_WARNING: return "WARNING";
    default: return "UNKNOWN";
  }
}

const char* eventName(Event event) {
  switch (event) {
    case E_TIMER: return "TIMER";
    case E_PED: return "PEDESTRIAN";
    case E_SERVICE: return "SERVICE";
    default: return "UNKNOWN";
  }
}

void setState(State newState, unsigned long duration) {
  currentState = newState;
  stateStart = millis();
  stateDuration = duration;

  digitalWrite(RED_PIN, LOW);
  digitalWrite(YELLOW_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);

  if (newState == S_GREEN)
    digitalWrite(GREEN_PIN, HIGH);

  else if (newState == S_YELLOW)
    digitalWrite(YELLOW_PIN, HIGH);

  else if (newState == S_RED)
    digitalWrite(RED_PIN, HIGH);

  else if (newState == S_WARNING) {
    yellowState = true;
    digitalWrite(YELLOW_PIN, HIGH);
    blinkStart = millis();
  }

  Serial.print(millis());
  Serial.print(" ms | STATE: ");
  Serial.println(stateName(newState));
}

void greenTimer() {
  setState(S_YELLOW, YELLOW_TIME);
}

void greenPed() {
  pedRequest = true;
}

void yellowTimer() {
  if (pedRequest) {
    pedRequest = false;
    setState(S_RED, RED_TIME + PED_EXTRA);
  }
  else {
    setState(S_RED, RED_TIME);
  }
}

void redTimer() {
  setState(S_GREEN, GREEN_TIME);
}

void serviceOn() {
  pedRequest = false;
  setState(S_WARNING, 0);
}

void serviceOff() {
  setState(S_GREEN, GREEN_TIME);
}

void setupTable() {
  for (int s = 0; s < NUM_STATES; s++)
    for (int e = 0; e < NUM_EVENTS; e++)
      table[s][e] = NULL;

  table[S_GREEN][E_TIMER] = greenTimer;
  table[S_GREEN][E_PED] = greenPed;
  table[S_GREEN][E_SERVICE] = serviceOn;

  table[S_YELLOW][E_TIMER] = yellowTimer;
  table[S_YELLOW][E_SERVICE] = serviceOn;

  table[S_RED][E_TIMER] = redTimer;
  table[S_RED][E_SERVICE] = serviceOn;

  table[S_WARNING][E_SERVICE] = serviceOff;
}

void processEvent(Event event) {
  Handler handler = table[currentState][event];

  if (handler != NULL) {
    Serial.print(millis());
    Serial.print(" ms | EVENT: ");
    Serial.println(eventName(event));

    handler();
  }
}

void setup() {
  pinMode(RED_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);

  pinMode(PED_BUTTON, INPUT_PULLUP);
  pinMode(SERVICE_BUTTON, INPUT_PULLUP);

  Serial.begin(9600);

  setupTable();
  setState(S_GREEN, GREEN_TIME);
}

void loop() {
  bool ped = digitalRead(PED_BUTTON);

  if (lastPed == HIGH && ped == LOW)
    processEvent(E_PED);

  lastPed = ped;

  bool service = digitalRead(SERVICE_BUTTON);

  if (lastService == HIGH && service == LOW)
    processEvent(E_SERVICE);

  lastService = service;

  if (currentState == S_WARNING) {
    if (millis() - blinkStart >= BLINK_TIME) {
      blinkStart = millis();
      yellowState = !yellowState;
      digitalWrite(YELLOW_PIN, yellowState);
    }
  }

  else if (millis() - stateStart >= stateDuration) {
    processEvent(E_TIMER);
  }
}
