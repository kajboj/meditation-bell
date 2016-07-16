#include <TM1637Display.h>

// Module connection pins (Digital Pins)
#define CLK 4
#define DIO 3
#define ULONG_MAX 4294967295
TM1637Display display(CLK, DIO);

typedef unsigned long Millis;
typedef unsigned long Seconds;

static const Millis DEBOUNCE_DELAY = 10;
static const Seconds MAXIMUM_SEC = 99*60 + 59;

typedef struct {
  Millis timeSinceLastPress;
  Millis delay;
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

typedef int KeyEvent;
static const KeyEvent NOTHING_HAPPENED = 0;
static const KeyEvent JUST_PRESSED     = 1;
static const KeyEvent JUST_RELEASED    = 2;

typedef int KeyState;
static const KeyState PRESSED  = HIGH;
static const KeyState RELEASED = LOW;

typedef struct {
  int pin;
  Millis lastDebounceTime;
  KeyState previousState;
  KeyState state;
  KeyEvent event;
  Millis lastTick;
  Millis lastPressTime;
} Key;

Key allKeys[] = {
  { 5 },
  { 6 },
};

static const int allKeyCount = sizeof(allKeys)/sizeof(Key);

Key *upKey = &allKeys[0];
Key *downKey = &allKeys[1];

Seconds previousSec;
Seconds remainingSec;
Millis startMillis;

Seconds getSec(Millis t) {
  return t / 1000;
}

void displayTime(Seconds sec) {
  display.showNumberDec(sec / 60, true, 2, 0);
  display.showNumberDec(sec % 60, true, 2, 2);
}

void resetTimer(Seconds remaining) {
  startMillis = millis();
  previousSec = 0;
  remainingSec = remaining;
  displayTime(remainingSec);
}

void updateTimer(Millis time) {
  if (remainingSec != 0) {
    Seconds sec = getSec(time - startMillis);

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

Millis findTickDelay(Millis timeSinceLastPress) {
  for(int i=0; i<allTickCount; i++) {
    Tick *tick = &allTicks[i];
    if (timeSinceLastPress < tick->timeSinceLastPress) {
      return tick->delay;
    }
  }
}

Seconds newRemaining(Seconds remainingSec) {
  return remainingSec + 60 - remainingSec % 60;
}

void makeTick(Millis time, Seconds (*newRemaining)(Seconds)) {
  upKey->lastTick = time;
  Seconds newRemainingSec = newRemaining(remainingSec);
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
  Millis time = millis();

  if (upKey->event == JUST_PRESSED) {
    upKey->lastPressTime = time;
    makeTick(time, newRemaining);
  } else {
    if (isPressed(upKey)) {
      Millis tickDelay = findTickDelay(time - upKey->lastPressTime);
      if (time - upKey->lastTick >= tickDelay) {
        makeTick(time, newRemaining);
      }
    }
  }

  updateTimer(time);
}
