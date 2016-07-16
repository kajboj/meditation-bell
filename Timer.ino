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
  { 1000,      128},
  { 2000,       64},
  { ULONG_MAX,  32},
};

static const int allTickCount = sizeof(allTicks)/sizeof(Tick);

typedef int KeyEvent;
static const KeyEvent NOTHING_HAPPENED = 0;
static const KeyEvent JUST_PRESSED     = 1;

typedef int KeyState;
static const KeyState PRESSED  = HIGH;
static const KeyState RELEASED = LOW;

typedef struct {
  int pin;
  Seconds (*newRemaining)(Seconds);
  Millis lastDebounceTime;
  KeyState previousState;
  KeyState state;
  KeyEvent event;
  Millis lastTick;
  Millis lastPressTime;
} Key;

Seconds newRemainingUp(Seconds remainingSec) {
  Seconds newRemaining = remainingSec + 60 - remainingSec % 60;
  if (newRemaining > MAXIMUM_SEC) {
    return 0;
  } else {
    return newRemaining;
  }
}

Seconds newRemainingDown(Seconds remainingSec) {
  Seconds step = remainingSec % 60;
  step = ((step == 0) ? 60 : step);
  if (step > remainingSec) {
    return MAXIMUM_SEC;
  } else {
    return remainingSec - step;
  }
}

Key allKeys[] = {
  { 5, &newRemainingUp },
  { 6, &newRemainingDown },
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

void resetTimer(Seconds remaining, Millis time) {
  startMillis = time;
  previousSec = 0;
  remainingSec = remaining;
  displayTime(remainingSec);
}

void updateTimer(Millis time) {
  if (remainingSec > 0) {
    Seconds sec = getSec(time - startMillis);

    if (sec != previousSec) {
      remainingSec -= sec - previousSec;
      displayTime(remainingSec);
      previousSec = sec;
    }
  }
}

void updateKeyEvents(Millis time) {
  for(int i=0; i<allKeyCount; i++) {
    Key *key = &allKeys[i];
    key->event = NOTHING_HAPPENED;

    int reading = digitalRead(key->pin);

    if (reading != key->previousState) {
      key->lastDebounceTime = time;
    }

    if ((time - key->lastDebounceTime) > DEBOUNCE_DELAY) {
      if (reading != key->state) {
        if ((reading == PRESSED) && (key->state == RELEASED)) {
          key->event = JUST_PRESSED;
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

void makeTick(Millis time, Seconds (*newRemaining)(Seconds)) {
  resetTimer(newRemaining(remainingSec), time);
}

boolean isPressed(Key *key) {
  return key->state == PRESSED;
}

void setup() {
  display.setBrightness(0x08);
  initializeKeys();
  resetTimer(0, millis());
}

void loop() {
  Millis time = millis();
  updateKeyEvents(time);

  for(int i=0; i<allKeyCount; i++) {
    Key *key = &allKeys[i];
    if (key->event == JUST_PRESSED) {
      key->lastPressTime = time;
      key->lastTick = time;
      makeTick(time, key->newRemaining);
    } else {
      if (isPressed(key)) {
        Millis tickDelay = findTickDelay(time - key->lastPressTime);
        if (time - key->lastTick >= tickDelay) {
          key->lastTick = time;
          makeTick(time, key->newRemaining);
        }
      }
    }
  }

  updateTimer(time);
}
