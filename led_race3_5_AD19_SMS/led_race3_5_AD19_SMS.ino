/*  
 * ____                     _      ______ _____    _____
  / __ \                   | |    |  ____|  __ \  |  __ \               
 | |  | |_ __   ___ _ __   | |    | |__  | |  | | | |__) |__ _  ___ ___ 
 | |  | | '_ \ / _ \ '_ \  | |    |  __| | |  | | |  _  // _` |/ __/ _ \
 | |__| | |_) |  __/ | | | | |____| |____| |__| | | | \ \ (_| | (_|  __/
  \____/| .__/ \___|_| |_| |______|______|_____/  |_|  \_\__,_|\___\___|
        | |                                                             
        |_|          
 Open LED Race
 An minimalist cars race for LED strip  
  
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.
 by gbarbarov@singulardevices.com  for Arduino day Seville 2019 
 Code made dirty and fast, next improvements in: 
 https://github.com/gbarbarov/led-race
 https://www.hackster.io/gbarbarov/open-led-race-a0331a
 https://twitter.com/openledrace
*/

#include <Adafruit_NeoPixel.h>

#define BOARD_TYPE      2           //Choose between 0=UNO, 1=NANO or 2=EVERY board

#define TRACK_LENGTH    MAXLED      //Default length of track used for race

#define NR_OF_PLAYERS   4           //Default number of players supported

//Do not edit below code unless required due to hardware change
#define PIN_LED A0  // R 500 ohms to DI pin for WS2812 and WS2813, for WS2813 BI pin of first LED to GND  ,  CAP 1000 uF to VCC 5v/GND,power supplie 5V 2A

#define BUTTON_PRESSED 0        //Button gives 0=low/1=high on press
#define P1 8    // switch player 1 to PIN and GND
#define PL1 6  // switch player 1 light
#define P2 7    // switch player 2 to PIN and GND
#define PL2 11   // switch player 2 light
#define P3 A1    // switch player 3 to PIN and GND
#define PL3 A3   // switch player 3 light
#define P4 A2    // switch player 4 to PIN and GND
#define PL4 A4   // switch player 4 light

// Color settings
#define BLUE track.Color(0, 0, 50)  //color: Blue
#define RED track.Color(50, 0, 0)  //color: Red
#define GREEN track.Color(0, 50, 0)  //color: Green
#define YELLOW track.Color(50, 20, 0) //color: Yellow

// Dim colors
#define D_BLUE track.Color(0, 0, 1)  //color: Blue
#define D_RED track.Color(1, 0, 0)  //color: Red
#define D_GREEN track.Color(0, 1, 0)  //color: Green
#define D_YELLOW track.Color(1, 1, 0)  //color: Yellow

// Status led colors
#define LED_PIN_BLUE    2
#define LED_PIN_GREEN   3
#define LED_PIN_RED     4

// Board specific configurations
#if BOARD_TYPE == 2
#define MAXLED          600     //MAX LEDs supported (=<300 for standard arduino with 2kB of memory)(Must be equal to or more than TRACK_LENGTH)
#define TIMES_TO_SAVE   10      //Number of different lap records to save. Note that increasing this will increase memory usage.
#define NR_OF_LAPS      5       //Default number of laps
#define OFFSET          41      //Clockspeed dependent compensation for better time accuracy
#define SERIAL          Serial1 //Serial1 is RX/TX on Nano EVERY board
#define DEF_DUAL_STRIP  true
#else
#define MAXLED          300
#define TIMES_TO_SAVE   7
#define SERIAL          Serial
#define NR_OF_LAPS      10      //Default number of laps
#define OFFSET          34
#define DEF_DUAL_STRIP  false
#endif

//define TIMER           timer0_millis //Better time compensation but not compatible with Nano EVERY boards

////////////////////////////////////////////////////////////////


Adafruit_NeoPixel track = Adafruit_NeoPixel(MAXLED, PIN_LED, NEO_GRB + NEO_KHZ800);

uint32_t dim_color[] = {D_BLUE, D_RED, D_GREEN, D_YELLOW};
uint32_t car_color[] = {BLUE, RED, GREEN, YELLOW};
byte player_pin[] = {P1, P2, P3, P4};
byte player_light_pin[] = {PL1, PL2, PL3, PL4};
uint32_t traffic_light[] = {GREEN, YELLOW, RED};

byte player[NR_OF_PLAYERS] = {0};
int btn_press[NR_OF_PLAYERS] = {0};
byte flag_sw[NR_OF_PLAYERS] = {1};
float speeds[NR_OF_PLAYERS] = {0};
float dist[NR_OF_PLAYERS] = {0};
byte laps[NR_OF_PLAYERS] = {0};

byte nr_of_players = NR_OF_PLAYERS; //Dynamic player set

unsigned long lap_times[NR_OF_PLAYERS] = {0};

struct highscore{
    byte best_race[TIMES_TO_SAVE];
    unsigned long best_time[TIMES_TO_SAVE];
};
highscore best_race_time = {0, 0};

int gravity_map[MAXLED];
int nr_of_races = 1;

byte nr_of_laps = NR_OF_LAPS; //total laps race
byte hill_mode = 1;
bool visible_hills = true;

int show_loop = 0;
int track_length = TRACK_LENGTH; // leds on track
bool dual_strip = DEF_DUAL_STRIP;

float ACEL = 0.2;
float kf = 0.015; //friction constant
float kg = 0.02; //gravity constant

byte flip = 0;
byte draworder = 0;

int com_code = 0;
bool new_code = false;
bool hold_race = true;
unsigned long timestamp = 0;
unsigned long previousMillsi = 0;

bool track_leader = true;
int8_t leader = -1;
int max_value = 0;
byte max_index=0;

bool altrn_serial = false;
HardwareSerial *MySerial = &SERIAL;

//millis correction variable
//extern volatile unsigned long TIMER;
unsigned long timer_offset = 0;

//Gravity calculation where 127 is normal gravity
//Arguments: H=ramp steepness, a=start of ramp (uphill), b = midpoint of ramp, c = end of ramp (downfall)
//Uphill calculation
//Start at 127 and reduce value down to H spread on distance b-a (lower value = more brake)
void set_ramp(byte H, byte a, byte b, byte c) {
    for(byte i = 0; i < (b - a); i++) {
        gravity_map[a + i] = 127 - i * ((float) H / (b - a));
    }
    
    //Down hill calculation
    //Start with 127+H and gradually fall down to 127 over distance c-b (higher value = more boost)
    gravity_map[b] = 127;
    for(byte i = 0; i < (c - b); i++) {
        gravity_map[b + i + 1] = 127 + H - i * ((float) H / (c - b));
    }
}

void com_commands();

void setup() {
    #if BOARD_TYPE==2
    Serial.begin(9600);
    Serial1.begin(9600);
    Serial.println(F("Running EVERY board configuration"));
    Serial.println(F("All data will be sent to BT"));
    Serial.println(F("Input 'x' to Switch to PC"));
    #else
    Serial.println("Running ATMEGA328 configuration"));
    #endif
    track.begin();
    for(byte i=0; i<NR_OF_PLAYERS; i++){
        player[i] = i+1;
        flag_sw[i] = 1;
        pinMode(player_pin[i], INPUT_PULLUP);
        pinMode(player_light_pin[i], OUTPUT);
        digitalWrite(player_light_pin[i], HIGH);
    }
    
    pinMode(2, OUTPUT);
    pinMode(3, OUTPUT);
    pinMode(4, OUTPUT);
    digitalWrite(2, HIGH);
    digitalWrite(3, HIGH);
    digitalWrite(4, HIGH);
    digitalWrite(2, LOW);
    delay(1000);
    digitalWrite(2, HIGH);
    digitalWrite(3, LOW);
    delay(1000);
    digitalWrite(3, HIGH);
    digitalWrite(4, LOW);
    delay(1000);
    digitalWrite(4, HIGH);
    digitalWrite(LED_PIN_RED, LOW);
    digitalWrite(LED_PIN_GREEN, LOW);
    delay(1000);
    digitalWrite(LED_PIN_RED, HIGH);
    digitalWrite(LED_PIN_GREEN, HIGH);

    for(int i = 0; i < track_length; i++) {
        gravity_map[i] = 127;
        track.setPixelColor(i, track.Color(0, 0, (127 - gravity_map[i]) / 8));
    }

    set_ramp(2, 50, 70, 90);
    set_ramp(2, 200, 222, 252);

    track.show();
    prepare_race();
}

void prepare_race() {
    show_race_info();
    track.clear();
    track.show();
    digitalWrite(LED_PIN_RED, HIGH);
    digitalWrite(LED_PIN_GREEN, HIGH);
    digitalWrite(LED_PIN_BLUE, HIGH);
    for(int8_t i=2; i>=0; i--){
        delay(2000);
        if(i==2)MySerial->println(F("Ready!"));
        if(i==1)MySerial->println(F("Set!"));
        if(i==0)MySerial->print(F("and..."));
        track.clear();
        track.setPixelColor(i*4, traffic_light[i]);
        track.setPixelColor(i*4+1, traffic_light[i]);
        track.setPixelColor(i*4+2, traffic_light[i]);
        track.setPixelColor(i*4+3, traffic_light[i]);
        track.show();
    }
    byte p=0;
    //Wait here until a button is pressed
    while(hold_race){
        if(digitalRead(player_pin[p]) == BUTTON_PRESSED && p <= nr_of_players) hold_race=false;
        p++;
        if(p >= nr_of_players) p = 0;
        if(Serial.available() > 0){
            com_code = Serial.read();
            if(com_code != char(13) && com_code != char(10)){
                MySerial->println("");
                com_commands();
            }
        }
        
        if(Serial1.available() > 0){
            com_code = Serial1.read();
            if(com_code != char(13) && com_code != char(10)){
                MySerial->println("");
                com_commands();
            }
        }
    }
    MySerial->println(F("Go!"));
    for(byte i=0; i<NR_OF_PLAYERS; i++){
        lap_times[i] = millis();
    }
    previousMillsi = millis();
}

void show_race_info(){
    delay(10);
    MySerial->println(F("--------------"));
    delay(20);
    MySerial->print(F("Race nr: "));
    MySerial->println(nr_of_races);
    MySerial->print(F("Number of laps: "));
    MySerial->println(nr_of_laps);
    delay(10);
    MySerial->print(F("Best time: "));
    print_saved_time(false);
    MySerial->print(F("Gravity "));
    if (hill_mode == 0) MySerial->println(F("deactivated"));
    else if (hill_mode == 1) MySerial->println(F("Low"));
    else if (hill_mode == 2) MySerial->println(F("Medium"));
    else if (hill_mode == 3) MySerial->println(F("High"));
    if(visible_hills) MySerial->print(F("Show "));
    else MySerial->print(F("Hide "));
    MySerial->println(F("hills"));
    delay(10);
    MySerial->print(F("Track length: "));
    MySerial->println(track_length);
    delay(10);
    MySerial->print(F("Nr of players: "));
    MySerial->println(nr_of_players);
    delay(10);
}

void print_saved_time(bool all_race_laps){
    int8_t array_index = -1;
    if(all_race_laps) MySerial->println(F("Laps - Time"));
    for(byte i=0; i<TIMES_TO_SAVE; i++){
        if(all_race_laps){
            MySerial->print(best_race_time.best_race[i]);
            MySerial->print(F(" - "));
            MySerial->println(best_race_time.best_time[i] * 0.001);
        }
        else if(best_race_time.best_race[i] == nr_of_laps){
            MySerial->println(best_race_time.best_time[i] * 0.001);
            array_index = i;
            break;
        }
    }
    if(!all_race_laps && array_index == -1){
        MySerial->println("---");
    }
    delay(10);
}

void clear_saved_time(){
    for(byte i=0; i<TIMES_TO_SAVE; i++){
        best_race_time.best_time[i] = 0;
        best_race_time.best_race[i] = 0;
    }
}

void blink_controller(byte player){
    for(byte i=0; i<2; i++){
        digitalWrite(player_light_pin[player], HIGH);
        delay(250);
        digitalWrite(player_light_pin[player], LOW);
        delay(250);
    }
    digitalWrite(player_light_pin[player], HIGH);
}

void draw_car(byte car) {
    for(int i = 0; i <= laps[car]; i++) {
        track.setPixelColor(((word) dist[car] % track_length) + i, car_color[car]);
    }
}

void button_register(byte car){
    byte pin;
    pin = digitalRead(player_pin[car]);

    if((flag_sw[car] == 1) && (pin == BUTTON_PRESSED)) {
        flag_sw[car] = 0;
        speeds[car] += ACEL;
        btn_press[car]++;
    }
    if((flag_sw[car] == 0) && (pin != BUTTON_PRESSED)) {
        flag_sw[car] = 1;
    }
}

void car_action(byte car){
    if((gravity_map[(word) dist[car] % track_length]) < 127 && hill_mode != 0) {
        if(speeds[car] > 2.0) {
            speeds[car] -= 0.3;
        }
        speeds[car] -= kg * (127 - (gravity_map[(word) dist[car] % track_length]));
    }
    if((gravity_map[(word) dist[car] % track_length]) > 127 && hill_mode != 0)
        speeds[car] += kg * ((gravity_map[(word) dist[car] % track_length]) - 127);

    speeds[car] -= speeds[car] * kf;
}

void lap_pass(byte car){
    if(laps[car] > 0){
        MySerial->print("P");
        MySerial->print(player[car]);
        MySerial->print(" lap: ");
        MySerial->print(laps[car]);
        MySerial->print("/");
        MySerial->print(nr_of_laps);
        MySerial->print(" - ");
        MySerial->println(((millis()+timer_offset)-lap_times[car]) * 0.001);
        //MySerial->print("offset: ");      //Timer calibration debug info
        //MySerial->println(timer_offset);  //Timer calibration debug info
        lap_times[car] = millis()+timer_offset;
    }
    laps[car]++;
}

void restart_race(){
    for(byte i=0; i<NR_OF_PLAYERS; i++){
        laps[i] = 0;
        dist[i] = 0;
        speeds[i] = 0;
        flag_sw[i] = 1;
        btn_press[i] = 0;
    }
    leader = -1;
    max_value = 0;
    max_index = 0;
    hold_race = true;
    timestamp = 0;
    timer_offset = 0;
    prepare_race();
}

void check_best_time(unsigned long race_time){
    int8_t array_index = -1;
    for(byte i=0; i<TIMES_TO_SAVE; i++){
        if(best_race_time.best_race[i] == nr_of_laps || best_race_time.best_race[i] == 0){
            array_index = i;
            break;
        }
    }
    if(array_index == -1) MySerial->println(F("No more space to store time"));
    else if(best_race_time.best_time[array_index] == 0 || race_time < best_race_time.best_time[array_index]){
            best_race_time.best_time[array_index] = race_time;
            MySerial->println(F("*** New record ***"));
            best_race_time.best_race[array_index] = nr_of_laps;
    }
    MySerial->print(F("time: "));
    MySerial->println(race_time * 0.001);
    delay(25);
}

void winner(byte player, unsigned long race_time){
    race_time = race_time - previousMillsi;
    MySerial->println(F("--------------"));
    MySerial->print(F("P"));
    delay(10);
    MySerial->print(player+1);
    MySerial->println(F(" Winner!"));
    delay(10);
    MySerial->print(F("Button press: "));
    MySerial->println(btn_press[player]);
    delay(20);
    check_best_time(race_time);
    theaterChase(car_color[player], 50);
    nr_of_races++;
}

void com_commands(){
    switch(com_code){
        case '0': //Restart race
            MySerial->println(F("Restart Race"));
            restart_race();
        break;
        case '1': //Increse laps
            MySerial->print(F("Increase lap: "));
            if(nr_of_laps<100) nr_of_laps++;
            MySerial->println(nr_of_laps);
        break;
        case '2': //Decrease laps
            MySerial->print(F("decrease lap: "));
            if(nr_of_laps>1) nr_of_laps--;
            MySerial->println(nr_of_laps);
        break;
        case '3': //Show hills on tracks
            visible_hills = !visible_hills;
            if (visible_hills) MySerial->println(F("Show hills!"));
            else MySerial->println(F("Hide hills!"));
            delay(3);
        break;
        case '4': //Enable/disable hills
            if(hill_mode < 3) hill_mode++;
            else hill_mode = 0;
            MySerial->println(hill_mode);
            //hills_activated = false;
            if (hill_mode == 0){
                MySerial->println(F("No Gravity!"));
            }
            else if (hill_mode == 1){
                MySerial->println(F("Low Gravity!"));
                kg = 0.02;
            }
            else if (hill_mode == 2){
                MySerial->println(F("Medium Gravity!"));
                kg = 0.05;
            }
            else if (hill_mode == 3){
                MySerial->println(F("High Gravity!"));
                kg = 0.07;
            }
        break;
        case '5': //Show current game setting information
            MySerial->println(F("Game info:"));
            show_race_info();
        break;
        case '6': //show all race records
            print_saved_time(true);
        break;
        case '7':
            nr_of_players++;
            if(nr_of_players > NR_OF_PLAYERS){
                nr_of_players = 1;
                digitalWrite(player_light_pin[1], LOW);
                digitalWrite(player_light_pin[2], LOW);
                digitalWrite(player_light_pin[3], LOW);
            }
            MySerial->print(F("Nr of players: "));
            MySerial->println(nr_of_players);
            if(nr_of_players != 1) blink_controller(nr_of_players - 1);
        break;
        case '8': //Change track length
            if(dual_strip) track_length = 300;
            else track_length = 600;
            dual_strip = !dual_strip;
            MySerial->println(F("Track length set to: "));
            MySerial->println(track_length);
        break;
        case 'c': //Clear time results
            MySerial->println(F("Clear records"));
            clear_saved_time();
            print_saved_time(true);
        break;
        case 'x': //Change serial output
            if(altrn_serial){
                MySerial->println(F("Sending to BT"));
                MySerial = &Serial1;
                MySerial->println(F("Sending to BT"));
                altrn_serial = false;
            }
            else{
                MySerial->println(F("Sending to USB"));
                MySerial = &Serial;
                MySerial->println(F("Sending to USB"));
                altrn_serial = true;
            }
        break;
        default:
            MySerial->print(F("unknown command: "));
            MySerial->print(com_code);
    }
    com_code=0;
}

void loop() {
    if(SERIAL.available() > 0){
        com_code = SERIAL.read();
        if(com_code != char(13) && com_code != char(10))
            com_commands();
    }


    if(track_leader && leader >= 0){
        digitalWrite(LED_PIN_RED, HIGH);
        digitalWrite(LED_PIN_GREEN, HIGH);
        digitalWrite(LED_PIN_BLUE, HIGH);
        switch(leader){
            case 0:
                digitalWrite(LED_PIN_BLUE, LOW);
            break;
            case 1:
                digitalWrite(LED_PIN_RED, LOW);
            break;
            case 2:
                digitalWrite(LED_PIN_GREEN, LOW);
            break;
            case 3:
                digitalWrite(LED_PIN_GREEN, LOW);
                digitalWrite(LED_PIN_RED, LOW);
            break;
        }
    }

    track.clear();
    if(visible_hills && hill_mode != 0){
        for(int i = 0; i < track_length; i++) {
            if((gravity_map[(word) i % track_length]) > 127)
                track.setPixelColor(i, track.Color(0, 5, 0));
            if((gravity_map[(word) i % track_length]) < 127)
                track.setPixelColor(i, track.Color(5, 0, 5));
        }
    }

    for(byte i=0; i<nr_of_players; i++){
        button_register(i);
        car_action(i);
        dist[i] += speeds[i];
    }

    if(track_leader){
        for(byte i = 0; i < nr_of_players; i++)
        {
            if(dist[i] > max_value) {
                max_value = dist[i];
                max_index = i;
            }
        }
        if(max_index != leader){
            MySerial->print(F("leader: P"));
            MySerial->println(max_index+1);
            leader = max_index;
        }
    }

    for(byte i=0; i<nr_of_players; i++){
        if(dist[i] > track_length*laps[i]) lap_pass(i);
        if(laps[i] > nr_of_laps){
            timestamp = millis()+timer_offset;
            winner(i, timestamp);
            restart_race();
        }
        if(dist[i] < 0) dist[0] = 0;
    }

    if((millis() & 256) == (256 * flip)) {
        if(flip == 0) flip = 1;
        else flip = 0;
        draworder++;
        if(draworder >= nr_of_players) draworder = 0;
    }

    if(draworder == 0) {
        draw_car(0);
        if(nr_of_players > 1) draw_car(1);
        if(nr_of_players > 2) draw_car(2);
        if(nr_of_players > 3) draw_car(3);
    } else if(draworder == 1){
        if(nr_of_players > 3) draw_car(3);
        if(nr_of_players > 2) draw_car(2);
        if(nr_of_players > 1) draw_car(1);
        draw_car(0);
    } else if(draworder == 2){
        if(nr_of_players > 1) draw_car(1);
        draw_car(0);
        if(nr_of_players > 3) draw_car(3);
        if(nr_of_players > 2) draw_car(2);
    } else{
        if(nr_of_players > 2) draw_car(2);
        if(nr_of_players > 3) draw_car(3);
        draw_car(0);
        if(nr_of_players > 1) draw_car(1);
    }
    
    track.show();
    //Since neopixel stops all interrupts offset is needed to be added to help keep time accurate
    //TIMER += MAXLED/34; //Roughly one ms should be added for each 34 led on strip
    timer_offset += MAXLED/OFFSET; //Roughly one ms should be added for each 34 led on strip
    interrupts();
}

// Theater-marquee-style chasing lights. Pass in a color (32-bit value,
// a la strip.Color(r,g,b) as mentioned above), and a delay time (in ms)
// between frames.
void theaterChase(uint32_t color, int wait) {
  int led_step=10;
  for(byte a=0; a<10; a++) {  // Repeat 10 times...
    for(byte b=0; b<led_step; b++) { //  'b' counts from 0 to 2...
      track.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in steps of 3...
      for(int c=b; c<track.numPixels(); c += led_step) {
        track.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      track.show(); // Update strip with new contents
      delay(wait);  // Pause for a moment
    }
  }
}
