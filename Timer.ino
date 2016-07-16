#include <TM1637Display.h>

// Module connection pins (Digital Pins)
#define CLK 4
#define DIO 3
#define ULONG_MAX 4294967295
TM1637Display display(CLK, DIO);

static const unsigned long DEBOUNCE_DELAY = 10;
static const unsigned long MAXIMUM_SEC = 99*60 + 59;

typedef struct {
  unsigned long timeSinceLastPress;
  unsigned long delay;
} Tick;

Tick allTicks[] = {
  { 1000,     256 },
  { 2000,     128 },
  { 3000,      64 },
  { 4000,      32 },
  { 5000,      16 },
  { 6000,       8 },
  { 7000,       4 },
  { 8000,       2 },
  { 9000,       1 },
  { ULONG_MAX,  0 },
};

static const int allTickCount = sizeof(allTicks)/sizeof(Tick);

typedef unsigned long Millis;

typedef int KeyEvent;
static const KeyEvent NOTHING_HAPPENED = 0;
static const KeyEvent JUST_PRESSED     = 1;
static const KeyEvent JUST_RELEASED    = 2;

typedef int KeyState;
static const KeyState PRESSED  = HIGH;
static const KeyState RELEASED = LOW;

typedef struct {
  int pin;
  unsigned long lastDebounceTime;
  KeyState previousState;
  KeyState state;
  KeyEvent event;
  unsigned long lastTick;
  unsigned long lastPressTime;
} Key;

Key allKeys[] = {
  { 5 },
  { 6 },
};

static const int allKeyCount = sizeof(allKeys)/sizeof(Key);

Key *upKey = &allKeys[0];
Key *downKey = &allKeys[1];

unsigned long previousSec;
unsigned long remainingSec;
unsigned long startMillis;

unsigned long getSec(unsigned long t) {
  return t / 1000;
}

void displayTime(unsigned long sec) {
  display.showNumberDec(sec / 60, true, 2, 0);
  display.showNumberDec(sec % 60, true, 2, 2);
}

void resetTimer(unsigned long remaining) {
  startMillis = millis();
  previousSec = 0;
  remainingSec = remaining;
  displayTime(remainingSec);
}

void updateTimer(unsigned long time) {
  if (remainingSec != 0) {
    unsigned long sec = getSec(time - startMillis);

    if (sec != previousSec) {
      remainingSec -= sec - previousSec;
      displayTime(remainingSec);
      previousSec = sec;
    }
  }
}

void updateKeyEvents() {
  for(int i=0; i<allKeyCount; i++) {
    Key *key = &allKeys[i];
    key->event = NOTHING_HAPPENED;

    int reading = digitalRead(key->pin);

    if (reading != key->previousState) {
      key->lastDebounceTime = millis();
    }

    if ((millis() - key->lastDebounceTime) > DEBOUNCE_DELAY) {
      if (reading != key->state) {
        if ((reading == PRESSED) && (key->state == RELEASED)) {
          key->event = JUST_PRESSED;
        }

        if ((reading == RELEASED) && (key->state == PRESSED)) {
          key->event = JUST_RELEASED;
        }

        key->state = reading;
      }
    }

    key->previousState = reading;
  }
}

void initializeKeys() {
  for(int i=0; i<allKeyCount; i++) {
    allKeys[i].lastDebounceTime = 0;
    allKeys[i].previousState = RELEASED;
    allKeys[i].state = RELEASED;
    allKeys[i].event = NOTHING_HAPPENED;
    allKeys[i].lastTick = 0;
    pinMode(allKeys[i].pin, INPUT);
  }
}

unsigned long findTickDelay(unsigned long timeSinceLastPress) {
  for(int i=0; i<allTickCount; i++) {
    Tick *tick = &allTicks[i];
    if (timeSinceLastPress < tick->timeSinceLastPress) {
      return tick->delay;
    }
  }
}

unsigned long newRemaining(unsigned long remainingSec) {
  return remainingSec + 60 - remainingSec % 60;
}

void makeTick(unsigned long time, unsigned long (*newRemaining)(unsigned long)) {
  upKey->lastTick = time;
  unsigned long newRemainingSec = newRemaining(remainingSec);
  if (newRemainingSec > MAXIMUM_SEC) {
    newRemainingSec = MAXIMUM_SEC;
  } else if (newRemainingSec < 0) {
    newRemainingSec = 0;
  }

  resetTimer(newRemainingSec);
}

boolean isPressed(Key *key) {
  return key->state == PRESSED;
}

void setup() {
  display.setBrightness(0x08);
  initializeKeys();
  resetTimer(0);
}

void loop() {
  updateKeyEvents();
  unsigned long time = millis();

  if (upKey->event == JUST_PRESSED) {
    upKey->lastPressTime = time;
    makeTick(time, newRemaining);
  } else {
    if (isPressed(upKey)) {
      unsigned long tickDelay = findTickDelay(time - upKey->lastPressTime);
      if (time - upKey->lastTick >= tickDelay) {
        makeTick(time, newRemaining);
      }
    }
  }

  updateTimer(time);
}
