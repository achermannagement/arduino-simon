typedef unsigned long time_t;

#define SIMON_LED0 2
#define SIMON_LED1 3
#define SIMON_LED2 4
#define SIMON_LED3 5

#define UNENTERED_VALUE -1
#define SIMON_PATTERN_LENGTH 12
#define LIGHT_BLINK_MILLI 400
#define PATTERN_SPACING_MILLI 2000
typedef struct {
  int8_t pattern[SIMON_PATTERN_LENGTH]; // correct pattern
  int8_t attempt[SIMON_PATTERN_LENGTH]; // stores user attempt
  int8_t curr_index_pattern;
  bool pattern_next;
  bool blank_all;
  time_t last_update;
  time_t now;
} simon_round_data;

simon_round_data data;

void setup() {
  randomSeed(analogRead(0)); // assumes pin0 is unconnected for entropy sake
  time_t game_start = millis();
  for(int i = 0; i < SIMON_PATTERN_LENGTH; i++){
    data.attempt[i] = UNENTERED_VALUE;
    data.pattern[i] = random(SIMON_LED0, SIMON_LED3 + 1); // assumes SIMON_LEDs are sequential
  }
  data.curr_index_pattern = 0;
  data.blank_all = true;
  data.last_update = millis();

  pinMode(SIMON_LED0, OUTPUT);
  pinMode(SIMON_LED1, OUTPUT);
  pinMode(SIMON_LED2, OUTPUT);
  pinMode(SIMON_LED3, OUTPUT);
}

void loop() {
  data.now = millis();
  if(data.blank_all) { // TODO: this could be a finite state machine
    if(data.now > data.last_update + LIGHT_BLINK_MILLI){
      data.pattern_next = true;
      data.blank_all = false;
    }
    for(int i = 0; i < SIMON_PATTERN_LENGTH; i++){
      digitalWrite(i, LOW); // not the most efficient way, but easy
    }
  } else if(data.pattern_next) {
    data.pattern_next = false;
    data.last_update = millis();
    digitalWrite(data.pattern[data.curr_index_pattern], HIGH);
    data.curr_index_pattern = (data.curr_index_pattern + 1) % SIMON_PATTERN_LENGTH;
  } else {
    // todo: set spacing between patterns
    if(data.now > data.last_update + LIGHT_BLINK_MILLI){
      data.blank_all = true;
      data.last_update = millis();
    }
  }
}
