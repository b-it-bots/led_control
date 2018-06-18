#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define PIN 6
#define NUM_LEDS 12

#define BUFFER_SIZE 128

#define COMMAND_OFF 0
#define COMMAND_FULL 1
#define COMMAND_ROTATE 2
#define COMMAND_BLINK 3
#define COMMAND_BLINK_LIMIT 4

#define DELAY 10

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

uint8_t command = COMMAND_OFF;
String command_data = "";
bool received_command = true;

String buffer = "";

void setup() {
    Serial.begin(9600);
    pixels.begin();
    pixels.show();

    //command = COMMAND_BLINK_LIMIT;
    //command_data = "50 255 0 0 5";
}

uint32_t mul_color(uint32_t color, float f);

void parse_buffer();
void command_full();
void command_rotate();
void command_blink();
void command_blink_limit();

void loop() {
    while(Serial.available() > 0) {
        uint8_t byte = Serial.read();
        if(byte == '\n') {
            parse_buffer();
            break;
        } else {
            buffer += (char) byte;
        }
    }
    switch(command) {
        case COMMAND_OFF:
            pixels.clear();
            pixels.show();
            break;
        case COMMAND_FULL:
            command_full();
            break;
        case COMMAND_ROTATE:
            command_rotate();
            break;
        case COMMAND_BLINK:
            command_blink();
            break;
        case COMMAND_BLINK_LIMIT:
            command_blink_limit();
            break;
    }
    delay(DELAY);
}

uint32_t command_blink_color = 0;
uint8_t command_blink_steps = 0;

uint8_t command_blink_count = 0;
bool command_blink_state = false;

void command_blink() {
    if(received_command) {
        int speed, r, g, b;
        char chars[command_data.length()+1];
        command_data.toCharArray(chars, command_data.length()+1);
        sscanf(chars, "%d %d %d %d", &speed, &r, &g, &b);
        command_blink_color = pixels.Color(r, g, b);
        command_blink_steps = (int)((float)speed / (float) DELAY);
        received_command = false;
        command_blink_state = false;
    }
    if(command_blink_count == 0) {
        pixels.clear();
        if(command_blink_state) {
            for (uint16_t i = 0; i < pixels.numPixels(); i++) {
                pixels.setPixelColor(i, command_blink_color);
            }
        }
        command_blink_state = !command_blink_state;
        pixels.show();
    }
    command_blink_count = (command_blink_count + 1) % command_blink_steps;
}

uint8_t command_blink_limit_limit = 0;
uint8_t command_blink_limit_count = 0;

void command_blink_limit() {
    if(received_command) {
        int speed, r, g, b, limit;
        char chars[command_data.length()+1];
        command_data.toCharArray(chars, command_data.length()+1);
        sscanf(chars, "%d %d %d %d %d", &speed, &r, &g, &b, &limit);
        command_blink_color = pixels.Color(r, g, b);
        command_blink_steps = (int)((float)speed / (float) DELAY);
        command_blink_limit_limit = limit;
        command_blink_limit_count = 0;
        command_blink_state = false;
        received_command = false;
    }
    if(command_blink_count == 0 && (command_blink_limit_count < command_blink_limit_limit || !command_blink_state )) {
        pixels.clear();
        if(command_blink_state) {
            for (uint16_t i = 0; i < pixels.numPixels(); i++) {
                pixels.setPixelColor(i, command_blink_color);
            }
        }
        command_blink_state = !command_blink_state;
        pixels.show();
        command_blink_limit_count++;
    }
    command_blink_count = (command_blink_count + 1) % command_blink_steps;
}

uint8_t command_rotate_dir = 0;
uint8_t command_rotate_steps = 0;
uint32_t command_rotate_color = 0;

uint8_t command_rotate_pos = 0;
uint8_t command_rotate_count = 0;

void command_rotate() {
    if(received_command) {
        int dir, speed, r, g, b;
        char chars[command_data.length()+1];
        command_data.toCharArray(chars, command_data.length()+1);
        sscanf(chars, "%d %d %d %d %d", &dir, &speed, &r, &g, &b);
        command_rotate_color = pixels.Color(r, g, b);
        command_rotate_dir = (bool) dir ? -1 : 1;
        command_rotate_steps = (int)((float)speed / (float) DELAY);
        received_command = false;
    }
    if(command_rotate_count == 0) {
        pixels.clear();
        for (uint16_t i = 0; i < pixels.numPixels(); i++)
        {
            float sigma = 0.5f;
            float diff = min(((int)command_rotate_pos - (int)i), (int)pixels.numPixels() + (int)i - (int)command_rotate_pos);
            float f = (1.0f / sqrt(2.0f * PI * sigma)) * pow(EULER, -((diff*diff)/(2.0f*sigma)));

            pixels.setPixelColor(i, mul_color(command_rotate_color, f));
        }
        
        command_rotate_pos = (command_rotate_pos + 1) % NUM_LEDS;
        pixels.show();
    }
    command_rotate_count = (command_rotate_count + 1) % command_rotate_steps;
}

uint32_t command_full_color = 0;

void command_full() {
    if(received_command) {
        int r, g, b;
        char chars[command_data.length()+1];
        command_data.toCharArray(chars, command_data.length()+1);
        sscanf(chars, "%d %d %d", &r, &g, &b);

        received_command = false;

        command_full_color = pixels.Color(r, g, b);

        for (uint16_t i = 0; i < pixels.numPixels(); i++)
        {
            pixels.setPixelColor(i, command_full_color);
        }
        pixels.show();
    }
}

uint32_t mul_color(uint32_t color, float f) {
    uint32_t r, g, b;
    r = (color & 0xff0000) >> 16;
    g = (color & 0x00ff00) >> 8;
    b = (color & 0x0000ff) >> 0;
    return pixels.Color(f * r, f * g, f * b);
}

void parse_buffer() {
    buffer.trim();
    command = buffer.toInt();
    int index = buffer.indexOf(' ');
    if(index > 0) {
        command_data = buffer.substring(index + 1, buffer.length());
    } else {
        command_data = "";
    }
    
    buffer = "";
    received_command = true;
}