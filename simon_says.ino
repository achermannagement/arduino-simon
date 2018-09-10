//TODO: led mapping could be abstracted to be less trash
typedef unsigned long time_t;

const int8_t SIMON_LED0 = 2;
const int8_t SIMON_LED1 = 3;
const int8_t SIMON_LED2 = 4;
const int8_t SIMON_LED3 = 5;

// pushbuttons
const int8_t BUTTON0 = 9;
const int8_t BUTTON1 = 10;
const int8_t BUTTON2 = 11;
const int8_t BUTTON3 = 12;

const int8_t PIEZO = 8;
const int16_t PIEZO_FREQ = 100;

const int8_t LIGHTS = 4;
typedef struct {
  int8_t pin;
} light;

light lights[LIGHTS];

/**
 * Setup the lights
 */
void setup_lights(){
  lights[0].pin = SIMON_LED0;
  lights[1].pin = SIMON_LED1;
  lights[2].pin = SIMON_LED2;
  lights[3].pin = SIMON_LED3;
}

const int8_t SIMON_PATTERN_LENGTH = 4;        // how many steps in the 
const int16_t LIGHT_BLINK_MILLI = 400;        // led remains on for this long & delay between flashes
const int16_t PATTERN_SPACING_MILLI = 2000;   // time between pattern display in milliseconds
enum SIMON_STATE {
  RESET,
  FLASH,
  BLINK_WAIT,
  BLANK,
  PATTERN_SPACING
};
typedef struct {
  int8_t pattern[SIMON_PATTERN_LENGTH]; // correct pattern
  int8_t curr_index_pattern;
  SIMON_STATE state;
  time_t last_update;
} simon_round_data;

simon_round_data simon_data;

void blank_all() {
  for(int i = 0; i < LIGHTS; i++){
    digitalWrite(lights[i].pin, LOW);
  }
}

void setup_state(SIMON_STATE s, simon_round_data *data) {
  data->last_update = millis();
  data->state = s;
}

/**
 * Handles the display of the Simon pattern
 */
void handle_pattern(simon_round_data *data){
  time_t now = millis();

  switch (data->state) {
    case RESET:
    {
      blank_all();
      for(int i = 0; i < SIMON_PATTERN_LENGTH; i++){
        data->pattern[i] = random(LIGHTS);
      }
      data->curr_index_pattern = 0;
      data->state = FLASH;
      data->last_update = millis();
    }
    break;
    case FLASH:
    {
      // blink current light
      digitalWrite(lights[data->pattern[data->curr_index_pattern]].pin, HIGH);
      data->curr_index_pattern = (data->curr_index_pattern + 1) % SIMON_PATTERN_LENGTH;
      setup_state(BLINK_WAIT, data);
    }
    break;

    case BLINK_WAIT:
    {
      if(now > data->last_update + LIGHT_BLINK_MILLI){
        setup_state(BLANK, data);
      }
    }
    break;
    
    case BLANK:
    {
      blank_all();
      if(now > data->last_update + LIGHT_BLINK_MILLI){
        if(data->curr_index_pattern == 0) {
          setup_state(PATTERN_SPACING, data);
        } else {
          setup_state(FLASH, data);
        }
      }
    }
    break;

    case PATTERN_SPACING:
    {
      if(now > data->last_update + PATTERN_SPACING_MILLI){
        setup_state(FLASH, data);
      }
    }
    break;

    default:
    {
      // TODO: error recovery?
    }
  }
}
// buttons are pulled down, when no button pressed 0V, but pressed button 5V
const int8_t BUTTONS = 4;  
const int16_t DEBOUNCE_PERIOD = 100;
enum BUTTON_STATE {
  WAITING,
  BUTTON_DOWN,
  BUTTON_RELEASE
};
typedef struct {
  int8_t pin;
  BUTTON_STATE state;
  time_t last_update;
  time_t last_pressed;
  time_t now;
} button;

button buttons[BUTTONS];

void setup_buttons(){
  buttons[0].pin = BUTTON0;
  buttons[1].pin = BUTTON1;
  buttons[2].pin = BUTTON2;
  buttons[3].pin = BUTTON3;

  for(int i = 0; i < BUTTONS; i++){
    buttons[i].state = WAITING;
    buttons[i].last_update = 0;
    buttons[i].last_pressed = 0;
  }
}

/**
 * Handles the state of a given button
 */
void handle_button(button *curr); // functions with arguments need to be declared!!!
void handle_button(button *curr){
  time_t now = millis();
  switch(curr->state){
    case WAITING:
    {
      // wait until button has been pressed
      if(digitalRead(curr->pin) == HIGH){
        curr->state = BUTTON_DOWN;
        curr->last_update = now;
      }
    }
    break;

    case BUTTON_DOWN:
    {
      // wait until the button has been released AND the debounce period has elapsed
      if(digitalRead(curr->pin) == LOW && (now - curr->last_update > DEBOUNCE_PERIOD)){
        curr->state = BUTTON_RELEASE;
      }
    }
    break;

    case BUTTON_RELEASE:
    {
      // acknowledge completed button action
      // check for this state to identify a button press
      curr->state = WAITING;
    }
    break;

    default:
    {
      // TODO: error handling?
    }
    break;
  }
}
void handle_buttons(){
  for(int i = 0; i < BUTTONS; i++){
    handle_button(&(buttons[i]));
  }
}

enum GAME_STATE {
  GAME_RESET,
  PATTERN,
  USER_ENTRY,
  LOSE,
  WIN
};

const int8_t UNENTERED_VALUE = -1;
typedef struct {
  GAME_STATE state;
  time_t start;
  int8_t attempt[SIMON_PATTERN_LENGTH]; // stores user attempt
  int8_t attempt_index;

  // input entry
  bool display_input;
  int8_t curr_input;
} game_data;
game_data data;

void reset_attempt(){
  data.attempt_index = 0;
  for(int i = 0; i < SIMON_PATTERN_LENGTH; i++){
    data.attempt[i] = UNENTERED_VALUE;
  }
}

void handle_entry() {
  int pressed;
  // iterate over buttons until we hit the pressed one
  for(pressed = 0; (pressed < BUTTONS) && (buttons[pressed].state != BUTTON_RELEASE); pressed++){}
  // highlight selected LED
  blank_all();
  digitalWrite(SIMON_LED0 + pressed, HIGH);
  if(pressed != BUTTONS){ // check one is pressed
    data.attempt[data.attempt_index] = pressed;
    data.attempt_index++;
    buttons[pressed].last_pressed = millis();
    data.display_input = true;
    data.curr_input = pressed;
    if(data.attempt_index == SIMON_PATTERN_LENGTH){
      bool correct = true;
      for(int i = 0; i < SIMON_PATTERN_LENGTH; i++){
        if(data.attempt[i] != simon_data.pattern[i]){
          correct = false;
        }
      }
      if(correct){
        data.state = WIN;
      } else {
        data.state = LOSE;
      }
      reset_attempt();
    }
  }
}

void handle_game() {
  time_t now = millis();
  handle_buttons(); // update all button states
  switch (data.state) {
    case GAME_RESET:
    {
      data.start = millis();
      simon_data.state = RESET;
      for(int i = 0; i < BUTTONS; i++){
        buttons[i].state = WAITING;
      }
      for(int i = 0; i < SIMON_PATTERN_LENGTH; i++){
        data.attempt[i] = UNENTERED_VALUE;
      }
      reset_attempt();
      data.display_input = false;
      data.curr_input = 0;
      data.state = PATTERN;
    }
    break;

    case PATTERN:
    {
      handle_pattern(&simon_data);
      for(int i = 0; i < BUTTONS; i++){
        if(buttons[i].state == BUTTON_RELEASE){
          data.state = USER_ENTRY;
          simon_data.state = RESET;
          handle_entry();
        }
      }
    }
    break;

    case USER_ENTRY:
    {
      handle_entry();

      if (data.display_input) {
        digitalWrite(lights[data.curr_input].pin, HIGH);
        if(now - buttons[data.curr_input].last_pressed > LIGHT_BLINK_MILLI){
          data.display_input = false;
          blank_all();
        }
      }
      int8_t count = 0;
      for(int i = 0; i < BUTTONS; i++){
        if(now - buttons[i].last_pressed > PATTERN_SPACING_MILLI) {
          count++;
        }
      }
      if(count == BUTTONS){ // check if any button has not been pressed for at least two seconds
        data.state = PATTERN;
        // clear entered user pattern
        for(int i = 0; i < SIMON_PATTERN_LENGTH; i++){
          data.attempt[i] = UNENTERED_VALUE;
        }
        data.attempt_index = 0;
      }
    }
    break;

    case LOSE:
    {
      Serial.println("LOSE");
      tone(PIEZO, PIEZO_FREQ, LIGHT_BLINK_MILLI);
      delay(LIGHT_BLINK_MILLI);
      data.state = GAME_RESET;
    }
    break;

    case WIN:
    {
      Serial.println("WIN");
      data.state = GAME_RESET;
    }
    break;

    default:
    {
      // error out?
    }
  }
}

void setup() {
  randomSeed(analogRead(0)); // assumes pin0 is unconnected for entropy sake
  pinMode(SIMON_LED0, OUTPUT);
  pinMode(SIMON_LED1, OUTPUT);
  pinMode(SIMON_LED2, OUTPUT);
  pinMode(SIMON_LED3, OUTPUT);
  pinMode(BUTTON0, INPUT);
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);
  pinMode(BUTTON3, INPUT);
  data.state = GAME_RESET;

  // setup buttons & lights
  setup_buttons();
  setup_lights();

  // piezo
  pinMode(PIEZO, OUTPUT);
  digitalWrite(PIEZO, LOW);

  // for debugging
  Serial.begin(9600);
}

void loop() {
  handle_game();

  /*for(int i = 0; i < SIMON_PATTERN_LENGTH; i++){
    Serial.print(data.attempt[i]);
    Serial.print(" ");
  }
  Serial.println();*/
}
