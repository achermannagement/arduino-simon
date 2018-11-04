/**
 * Simon implemented on Arduino
    Copyright (C) 2018  Joshua Achermann

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    email: joshua.achermann@gmail.com
 */

typedef unsigned long time_t;

// led pins
const int8_t SIMON_LED0 = 2;
const int8_t SIMON_LED1 = 3;
const int8_t SIMON_LED2 = 4;
const int8_t SIMON_LED3 = 5;

// pushbutton pins
const int8_t BUTTON0 = 9;
const int8_t BUTTON1 = 10;
const int8_t BUTTON2 = 11;
const int8_t BUTTON3 = 12;

// piezo buzzer pin and tone frequency
const int8_t PIEZO = 8;
const int16_t PIEZO_FREQ = 100;

/****************************************************************************************************
 * LED
 */
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

/****************************************************************************************************
 * SIMON ROUND
 */
const int8_t SIMON_PATTERN_LENGTH = 4;        // how many steps in the
const int16_t LIGHT_BLINK_MILLI = 400;        // led remains on for this long & delay between flashes
const int16_t PATTERN_SPACING_MILLI = 2000;   // time between pattern display in milliseconds
enum SIMON_STATE {
  RESET,                  // the reset state
  FLASH,                  // an led is turned on
  BLINK_WAIT,             // wait for a period
  BLANK,                  // turn off all leds
  PATTERN_SPACING         // wait for a longer period
};
typedef struct {
  int8_t pattern[SIMON_PATTERN_LENGTH]; // correct & displayed pattern
  int8_t curr_index_pattern;            // current index in displaying pattern
  SIMON_STATE state;                    // state of simon round
  time_t last_update;                   // timestamp where last state update occurred
} simon_round_data;

simon_round_data simon_data;

/**
 * Turns all LEDs off
 */
void blank_all() {
  for(int i = 0; i < LIGHTS; i++){
    digitalWrite(lights[i].pin, LOW);
  }
}

/**
 * Turns all LEDs on
 */
void light_all() {
  for(int i = 0; i < LIGHTS; i++){
    digitalWrite(lights[i].pin, HIGH);
  }
}

/**
 * Updates the simon round data
 * s        state to progress to
 * data     round data
 */
 void setup_state(SIMON_STATE s, simon_round_data *data);
void setup_state(SIMON_STATE s, simon_round_data *data) {
  data->last_update = millis();
  data->state = s;
}

/**
 * Handles the display of the Simon pattern
 */
 void handle_pattern(simon_round_data *data);
void handle_pattern(simon_round_data *data){
  time_t now = millis();

  switch (data->state) {
    case RESET:
    {
      blank_all();
      data->curr_index_pattern = 0;
      data->last_update = millis();
      data->state = FLASH;
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
      // should never get here
      Serial.println("Pattern FSM error");
    }
  }
}

/****************************************************************************************************
 * BUTTONS
 */

/**
* Assumes buttons are active high
* When the button is pressed the pin should read HIGH, LOW otherwise
*/
const int8_t BUTTONS = 4;             // number of buttons
const int16_t DEBOUNCE_PERIOD = 100;  // how long to wait
enum BUTTON_STATE {
  WAITING,                            // wait for the button to be pressed
  BUTTON_DOWN,                        // wait for button to be released + debouncing
  BUTTON_RELEASE                      // acknowledge the button has been released
};
typedef struct {
  int8_t pin;                         // which pin to read button input from
  BUTTON_STATE state;                 // current state of button
  time_t last_update;                 // used to debounce
  time_t last_pressed;                // last time button was released
} button;

button buttons[BUTTONS];

/**
 * Setup buttons
 */
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
      // should never get here
      Serial.println("Button FSM error");
    }
    break;
  }
}

/**
 * Handles the update of all button states
 */
void handle_buttons(){
  for(int i = 0; i < BUTTONS; i++){
    handle_button(&(buttons[i]));
  }
}

/****************************************************************************************************
 * GAME
 */
enum GAME_STATE {
  GAME_RESET,                   // reset
  PATTERN,                      // currently displaying pattern
  USER_ENTRY,                   // currently accepting and displaying input
  LOSE,                         // player has lost
  WIN                           // player has won
};

const int8_t VICTORY_ITER = 3; // number of flashes in win state
const int8_t UNENTERED_VALUE = -1;
typedef struct {
  GAME_STATE state;                       // current game state
  time_t start;                           // timestamp of game start
  int8_t attempt[SIMON_PATTERN_LENGTH];   // stores user attempt
  int8_t attempt_index;                   // current user attempt index

  // input entry
  bool display_input;                     // whether to display light selected
  int8_t curr_input;                      // user entered light
} game_data;
game_data data;

/**
 * Reset user entered attempt
 */
void reset_attempt(){
  data.attempt_index = 0;
  data.curr_input = 0;
  for(int i = 0; i < SIMON_PATTERN_LENGTH; i++){
    data.attempt[i] = UNENTERED_VALUE;
  }
}

/**
 * Once a button has been detected to be released, we handle the result here
 */
void handle_entry() {
  int pressed;
  // iterate over buttons until we hit the pressed one
  for(pressed = 0; (pressed < BUTTONS) && (buttons[pressed].state != BUTTON_RELEASE); pressed++){}
  if(pressed != BUTTONS){ // check one is pressed
    // update the user attempt pattern with pressed button
    data.attempt[data.attempt_index] = pressed;
    data.attempt_index++;
    // update the last pressed timeframe on the pressed button
    buttons[pressed].last_pressed = millis();
    // update the light to display
    data.display_input = true;
    data.curr_input = pressed;
    // if we have a full attempt pattern, compare it to the correct pattern
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

/**
 * Handle the game finite state machine
 */
void handle_game() {
  time_t now = millis();
  handle_buttons(); // update all button states
  switch (data.state) {
    case GAME_RESET:
    {
      data.start = millis();
      for(int i = 0; i < BUTTONS; i++){
        buttons[i].state = WAITING;
      }
      for(int i = 0; i < SIMON_PATTERN_LENGTH; i++){
        simon_data.pattern[i] = random(LIGHTS);
      }
      reset_attempt();
      data.display_input = false;
      data.state = PATTERN;
    }
    break;

    case PATTERN:
    {
      handle_pattern(&simon_data);
      // if user input detected, swap to USER_ENTRY state
      for(int i = 0; i < BUTTONS; i++){
        if(buttons[i].state == BUTTON_RELEASE){
          blank_all();
          data.state = USER_ENTRY;
          simon_data.state = FLASH;
          handle_entry();
        }
      }
    }
    break;

    case USER_ENTRY:
    {
      handle_entry();
      // if we are displaying input currently, display the current input
      if (data.display_input) {
        digitalWrite(lights[data.curr_input].pin, HIGH);
        if(now - buttons[data.curr_input].last_pressed > LIGHT_BLINK_MILLI){
          data.display_input = false;
          blank_all();
        }
      }
      // check if any button has not been pressed for at least two seconds
      int8_t count = 0;
      for(int i = 0; i < BUTTONS; i++){
        if(now - buttons[i].last_pressed > PATTERN_SPACING_MILLI) {
          count++;
        }
      }
      if(count == BUTTONS){ // check if any button has not been pressed for at least two seconds
      if(count == BUTTONS){
        // go back to displaying the pattern
        simon_data.state = RESET;
        data.state = PATTERN;
        // clear entered user pattern
        reset_attempt();
      }
    }
    break;

    case LOSE:
    {
      Serial.println("LOSE");
      // play a losing tone
      tone(PIEZO, PIEZO_FREQ, LIGHT_BLINK_MILLI);
      delay(LIGHT_BLINK_MILLI);
      data.state = PATTERN;
    }
    break;

    case WIN:
    {
      Serial.println("WIN");
      data.state = GAME_RESET;
      simon_data.state = RESET;

      //TODO: make this non-blocking
      for(int i = 0; i < VICTORY_ITER; i++){
        light_all();
        delay(LIGHT_BLINK_MILLI);
        blank_all();
        delay(LIGHT_BLINK_MILLI);
      }
    }
    break;

    default:
    {
      // should never get here
      Serial.println("Game FSM error");
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
  simon_data.state = RESET;

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
}
