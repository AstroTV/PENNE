#include <Servo.h>
#include "Freenove_WS2812B_RGBLED_Controller.h"

#define ACTION_MOVE         'A'
#define ACTION_BUZZER       'D'
#define ACTION_LIGHT        'C'
#define ACTION_ALIVE        'E'
#define ACTION_LOCK         'L'
#define ACTION_GEAR         'G'

#define PIN_SERVO           2
#define PIN_DIRECTION_LEFT  4
#define PIN_DIRECTION_RIGHT 3
#define PIN_MOTOR_PWM_LEFT  6
#define PIN_MOTOR_PWM_RIGHT 5
#define PIN_SONIC_TRIG      7
#define PIN_SONIC_ECHO      8
#define MOTOR_PWM_DEAD      10
#define PIN_BUZZER          A0

#define COMMANDS_COUNT_MAX  8
#define INTERVAL_CHAR       '#'

#define OBSTACLE_DISTANCE   20 // cm
#define MAX_DISTANCE        300 // cm
#define SONIC_TIMEOUT       (MAX_DISTANCE*60)
#define SOUND_VELOCITY      340 // m/s

#define STRIP_I2C_ADDRESS   0x20
#define STRIP_LEDS_COUNT    10
#define RGB_MODE_OFF        0
#define RGB_MODE_LIGHT      1
#define RGB_MODE_HIGH_BEAM  2
#define RGB_MODE_LEFT_TS    3
#define RGB_MODE_RIGHT_TS   4
#define RGB_MODE_WARNING    5
#define RGB_MODE_BRAKE      6
#define RGB_MODE_TURN_SIGN_OFF  7

/* LEDs */
#define NUMBER_OF_LEDS      10
#define LED_RIGHT_FRONT_TS  0
#define LED_RIGHT_FRONT_L   1
#define LED_HIGH_BEAM       2
#define LED_LEFT_FRONT_L    3           
#define LED_LEFT_FRONT_TS   4
#define LED_LEFT_BACK_TS    5
#define LED_LEFT_BACK_L     6
#define LED_BRAKE           7
#define LED_RIGHT_BACK_L    8
#define LED_RIGHT_BACK_TS   9

#define RED     0
#define GREEN   1
#define BLUE    2

/* Input variables */
String inputStringBLE = "";
int parameters[COMMANDS_COUNT_MAX], parameterCount = 0;
bool stringComplete = false;

/* Buzzer variables */
bool isBuzzered = false;

/* Servo variables */
Servo servo;
int pos = 90;

/* RGB variables */
Freenove_WS2812B_Controller strip(STRIP_I2C_ADDRESS, STRIP_LEDS_COUNT, TYPE_GRB);
u8 stripMode = RGB_MODE_OFF;
bool flag_turnSignLeft = false;
bool flag_turnSignRight = false;
bool flag_warning = false;
bool state_turnSignLeft = false;
bool state_turnSignRight = false;
bool state_warning = false; 
unsigned long previousMillis = 0;
const long interval = 500;
u8 tmp_leds[NUMBER_OF_LEDS][3] = { 0 };
bool flag_reverse = false;
bool flag_lightsout = true;

/* Check connection variables */
bool flag_alive = false;
bool flag_connection_lost = false;
unsigned long previousAlive = 0;
const long intervalAlive = 1500;

/* Obstacle variables */
bool flag_obstacle = false;

/**
 * 
*/
void setup()
{
    pinsSetup();
    servo.write(pos);
    servo.attach(PIN_SERVO);
    strip.begin();
    strip.setAllLedsColorData(0, 0, 0); // lights out
    Serial.begin(9600);
}

/**
 * 
*/
void loop()
{
    if(stringComplete)
    {
        //Serial.println(inputStringBLE);
        String inputCommandArray[COMMANDS_COUNT_MAX];

        String inputStringTemp = inputStringBLE;
        for(u8 i = 0; i < COMMANDS_COUNT_MAX; i++)
        {
            int index = inputStringTemp.indexOf(INTERVAL_CHAR);
            if(index < 0)
            {
                break;
            }
            parameterCount = i;
            inputCommandArray[i] = inputStringTemp.substring(0, index);
            inputStringTemp = inputStringTemp.substring(index + 1);
            parameters[i] = inputCommandArray[i].toInt();
        }
        stringComplete = false;
        inputStringBLE = "";

        char commandChar = inputCommandArray[0].charAt(0);

        // evaluate command
        switch(commandChar)
        {
            case ACTION_MOVE:
                if(parameterCount == 2)
                {
                    Serial.print(parameters[1]);
                    Serial.print(" - ");
                    Serial.println(parameters[2]);
                    motorRun(parameters[1], parameters[2]);
                }
                else
                {
                    motorRun(0, 0);
                }
                break;
            case ACTION_BUZZER:
                if(parameterCount == 1)
                {
                    setBuzzer(parameters[1]);
                }
                break;
            case ACTION_LIGHT:
                if(parameterCount == 2)
                {
                    stripMode = parameters[1];
                    setLeds();
                }
                break;
            case ACTION_ALIVE:
                if(parameterCount == 1)
                {
                    flag_alive = parameters[1];
                }
                break;
            case ACTION_LOCK:
                if(parameterCount == 1)
                {
                    lockCar(parameters[1]);
                }
                break;
            case ACTION_GEAR:
                if(parameterCount == 1)
                {
                    gearlight(parameters[1]);
                }
                break;
        }
    }

    // get current time in milliseconds
    unsigned long currentMillis = millis();

    // check if connection is alive
    if (currentMillis - previousAlive >= intervalAlive) 
    {
        // stop car for safety reasons and set flag_connection_lost
        if(!flag_alive && !flag_connection_lost)
        {
            stopCar();
            strip.setAllLedsColor(255, 0, 0);
            flag_connection_lost = true;
        }

        // check if car is now reconnected
        else if(flag_alive && flag_connection_lost)
        {
            strip.setAllLedsColor(0);
            restoreLeds();
            flag_connection_lost = false;
        }
        flag_alive = false;
        
        previousAlive = currentMillis;
    }

    // check if turnSign should blink
    if (currentMillis - previousMillis >= interval && (flag_warning || flag_turnSignLeft || flag_turnSignRight)) 
    {
        turnSignal();
        previousMillis = currentMillis;
    }

    // check if there is now enough space between car and obstacle
    if(flag_obstacle && getSonar() > OBSTACLE_DISTANCE)
    {
        strip.setAllLedsColor(0);
        restoreLeds();
        flag_obstacle = false;
    }
}

/**
 * 
*/
void serialEvent()
{
    while(Serial.available())
    {
        char inChar = (char)Serial.read();
        inputStringBLE += inChar;
        if(inChar == '\n')
        {
            stringComplete = true;
        }
    }
}

/**
 * 
*/
void pinsSetup()
{
    pinMode(PIN_DIRECTION_LEFT, OUTPUT);
    pinMode(PIN_MOTOR_PWM_LEFT, OUTPUT);
    pinMode(PIN_DIRECTION_RIGHT, OUTPUT);
    pinMode(PIN_MOTOR_PWM_RIGHT, OUTPUT);

    pinMode(PIN_SONIC_TRIG, OUTPUT);
    pinMode(PIN_SONIC_ECHO, INPUT);

    setBuzzer(false);
}

void setBuzzer(bool flag)
{
    isBuzzered = flag;
    pinMode(PIN_BUZZER, flag);
    digitalWrite(PIN_BUZZER, flag);
}

/**
 * 
*/
void setLeds()
{
    switch(stripMode)
    {
        case RGB_MODE_OFF:
            flag_lightsout = true;
            strip.setAllLedsColor(0, 0, 0);
            setAllLeds();
            break;
        case RGB_MODE_LIGHT:
            flag_lightsout = false;
            strip.setLedColor(LED_HIGH_BEAM, 0, 0, 0);
            strip.setLedColor(LED_RIGHT_FRONT_L, 255, 255, 51);
            strip.setLedColor(LED_LEFT_FRONT_L, 255, 255, 51);
            strip.setLedColor(LED_LEFT_BACK_L, 102, 0, 0);
            if(!flag_reverse) strip.setLedColor(LED_RIGHT_BACK_L, 102, 0, 0);
            else strip.setLedColor(LED_RIGHT_BACK_L, 255, 255, 153);
            setLedColor(LED_HIGH_BEAM, 0, 0, 0);
            setLedColor(LED_RIGHT_FRONT_L, 255, 255, 51);
            setLedColor(LED_LEFT_FRONT_L, 255, 255, 51);
            setLedColor(LED_LEFT_BACK_L, 102, 0, 0);
            if(!flag_reverse) setLedColor(LED_RIGHT_BACK_L, 102, 0, 0);
            else setLedColor(LED_RIGHT_BACK_L, 255, 255, 153);
            break;
        case RGB_MODE_HIGH_BEAM:
            flag_lightsout = false;
            strip.setLedColor(LED_RIGHT_FRONT_L, 255, 255, 153);
            strip.setLedColor(LED_HIGH_BEAM, 255, 255, 153);
            strip.setLedColor(LED_LEFT_FRONT_L, 255, 255, 153);
            strip.setLedColor(LED_LEFT_BACK_L, 102, 0, 0);
            if(!flag_reverse) strip.setLedColor(LED_RIGHT_BACK_L, 102, 0, 0);
            else strip.setLedColor(LED_RIGHT_BACK_L, 255, 255, 153);
            setLedColor(LED_RIGHT_FRONT_L, 255, 255, 153);
            setLedColor(LED_HIGH_BEAM, 255, 255, 153);
            setLedColor(LED_LEFT_FRONT_L, 255, 255, 153);
            setLedColor(LED_LEFT_BACK_L, 102, 0, 0);
            if(!flag_reverse) setLedColor(LED_RIGHT_BACK_L, 102, 0, 0);
            else setLedColor(LED_RIGHT_BACK_L, 255, 255, 153);
            break;
        case RGB_MODE_LEFT_TS:
            flag_turnSignLeft   = true;
            flag_turnSignRight  = false;
            flag_warning        = false;
            break;
        case RGB_MODE_RIGHT_TS:
            flag_turnSignRight  = true;
            flag_turnSignLeft   = false;
            flag_warning        = false;
            break;
        case RGB_MODE_TURN_SIGN_OFF:
            strip.setLedColor(LED_LEFT_FRONT_TS, 0, 0, 0);
            strip.setLedColor(LED_LEFT_BACK_TS, 0, 0, 0);
            strip.setLedColor(LED_RIGHT_FRONT_TS, 0, 0, 0);
            strip.setLedColor(LED_RIGHT_BACK_TS, 0, 0, 0);
            setLedColor(LED_LEFT_FRONT_TS, 0, 0, 0);
            setLedColor(LED_LEFT_BACK_TS, 0, 0, 0);
            setLedColor(LED_RIGHT_FRONT_TS, 0, 0, 0);
            setLedColor(LED_RIGHT_BACK_TS, 0, 0, 0);
            flag_turnSignLeft = false;
            flag_turnSignRight = false;
            flag_warning = false;
            break;
        case RGB_MODE_WARNING:
            flag_warning        = true;
            flag_turnSignLeft   = false;
            flag_turnSignRight  = false;
            break;
        case RGB_MODE_BRAKE:
            if(flag_connection_lost || flag_obstacle)
            {
                break;
            }
            if(parameters[2] == 1)
            {
                strip.setLedColor(LED_BRAKE, 255, 0, 0);
            }   
            else
            {
                strip.setLedColor(LED_BRAKE, 0, 0, 0);
            }
            break;
    }
}

/**
 * 
*/
void turnSignal()
{
    if(flag_obstacle || flag_connection_lost)
    {
        return;
    }

    if(flag_warning)
    {
        if(!state_warning)
        {
            state_warning = true;
            strip.setLedColor(LED_LEFT_FRONT_TS, 255, 128, 0);
            strip.setLedColor(LED_LEFT_BACK_TS, 255, 128, 0);
            strip.setLedColor(LED_RIGHT_FRONT_TS, 255, 128, 0);
            strip.setLedColor(LED_RIGHT_BACK_TS, 255, 128, 0);
        }
        else
        {
            state_warning = false;
            strip.setLedColor(LED_LEFT_FRONT_TS, 0, 0, 0);
            strip.setLedColor(LED_LEFT_BACK_TS, 0, 0, 0);
            strip.setLedColor(LED_RIGHT_FRONT_TS, 0, 0, 0);
            strip.setLedColor(LED_RIGHT_BACK_TS, 0, 0, 0);
        }
    }
    else if(flag_turnSignLeft)
    {
        strip.setLedColor(LED_RIGHT_FRONT_TS, 0, 0, 0);
        strip.setLedColor(LED_RIGHT_BACK_TS, 0, 0, 0);
        if(!state_turnSignLeft) 
        {
            state_turnSignLeft = true;
            strip.setLedColor(LED_LEFT_FRONT_TS, 255, 128, 0);
            strip.setLedColor(LED_LEFT_BACK_TS, 255, 128, 0);
        } 
        else
        {
            state_turnSignLeft = false;
            strip.setLedColor(LED_LEFT_FRONT_TS, 0, 0, 0);
            strip.setLedColor(LED_LEFT_BACK_TS, 0, 0, 0);
        }
    }
    else if(flag_turnSignRight)
    {
        strip.setLedColor(LED_LEFT_FRONT_TS, 0, 0, 0);
        strip.setLedColor(LED_LEFT_BACK_TS, 0, 0, 0);
        if(!state_turnSignRight) 
        {
            state_turnSignRight = true;
            strip.setLedColor(LED_RIGHT_FRONT_TS, 255, 128, 0);
            strip.setLedColor(LED_RIGHT_BACK_TS, 255, 128, 0);
        } 
        else 
        {
            state_turnSignRight = false;
            strip.setLedColor(LED_RIGHT_FRONT_TS, 0, 0, 0);
            strip.setLedColor(LED_RIGHT_BACK_TS, 0, 0, 0);
        }
    }
}

/**
 * 
*/
void motorRun(int speedL, int speedR)
{
    int dirL = 0, dirR = 0;

    if(speedL >= 0 && getSonar() > OBSTACLE_DISTANCE)
    {
        dirL = 0;
    }
    else if(speedL < 0)
    {
        dirL = 1;
        speedL = -speedL;
    }
    // car is too close to an obstacle
    else
    {
        speedL = 0;
        speedR = 0;
        strip.setAllLedsColor(0, 0, 255);
        flag_obstacle = true;
    }

    if(speedR > 0)
    {
        dirR = 1;
    }
    else
    {
        dirR = 0;
        speedR = -speedR;
    }

    speedL = constrain(speedL, 0, 255);
    speedR = constrain(speedR, 0, 255);

    if(abs(speedL) < MOTOR_PWM_DEAD && abs(speedR) < MOTOR_PWM_DEAD)
    {
        speedL = 0;
        speedR = 0;
    }

    digitalWrite(PIN_DIRECTION_LEFT, dirL);
    digitalWrite(PIN_DIRECTION_RIGHT, dirR);
    analogWrite(PIN_MOTOR_PWM_LEFT, speedL);
    analogWrite(PIN_MOTOR_PWM_RIGHT, speedR);
}

/**
 * 
*/
float getSonar()
{
    unsigned long pingTime;
    float distance;
    digitalWrite(PIN_SONIC_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_SONIC_TRIG, LOW);
    pingTime = pulseIn(PIN_SONIC_ECHO, HIGH, SONIC_TIMEOUT);

    if(pingTime != 0)
    {
        distance = (float)pingTime * SOUND_VELOCITY / 2 / 10000;
    }
    else
    {
        distance = MAX_DISTANCE;
    }
    return distance;
}

/**
 * 
*/
void stopCar()
{
    digitalWrite(PIN_DIRECTION_LEFT, 0);
    digitalWrite(PIN_DIRECTION_RIGHT, 0);
    analogWrite(PIN_MOTOR_PWM_LEFT, 0);
    analogWrite(PIN_MOTOR_PWM_RIGHT, 0);
    strip.setAllLedsColor(255, 0, 0);
}

/**
 * 
*/
void lockCar(int parameter)
{
    u8 number_loops;
    if(parameter == 0)
    {
        number_loops = 1;
    }
    else
    {
        number_loops = 2;
    }

    for(u8 cnt = 0; cnt < number_loops; cnt++)
    {
        // set LEDs
        strip.setLedColor(LED_LEFT_FRONT_TS, 255, 128, 0);
        strip.setLedColor(LED_LEFT_BACK_TS, 255, 128, 0);
        strip.setLedColor(LED_RIGHT_FRONT_TS, 255, 128, 0);
        strip.setLedColor(LED_RIGHT_BACK_TS, 255, 128, 0);
        setBuzzer(1);

        delay(100);

        // reset LEDs
        strip.setLedColor(LED_LEFT_FRONT_TS, 0, 0, 0);
        strip.setLedColor(LED_LEFT_BACK_TS, 0, 0, 0);
        strip.setLedColor(LED_RIGHT_FRONT_TS, 0, 0, 0);
        strip.setLedColor(LED_RIGHT_BACK_TS, 0, 0, 0);
        setBuzzer(0);

        delay(100);
    }
}

void setAllLeds()
{
    for(u8 cnt = 0; cnt < NUMBER_OF_LEDS; cnt++)
    {
        tmp_leds[cnt][RED]      = 0;
        tmp_leds[cnt][GREEN]    = 0;
        tmp_leds[cnt][BLUE]     = 0;
    }
}

void setLedColor(u8 index, u8 r, u8 g, u8 b)
{
    tmp_leds[index][RED]      = r;
    tmp_leds[index][GREEN]    = g;
    tmp_leds[index][BLUE]     = b;
}

void restoreLeds()
{
    for(u8 cnt = 0; cnt < NUMBER_OF_LEDS; cnt++)
    {
        strip.setLedColor(cnt, tmp_leds[cnt][RED], tmp_leds[cnt][GREEN], tmp_leds[cnt][BLUE]);
    }
}

void gearlight(int parameter)
{
    if(parameter == 1)              // gear = reverse
    {
        flag_reverse = true;
        strip.setLedColor(LED_RIGHT_BACK_L, 255, 255, 153);
        setLedColor(LED_RIGHT_BACK_L, 255, 255, 153);
    }
    else
    {
        flag_reverse = false;
        if(!flag_lightsout)
        {
            strip.setLedColor(LED_RIGHT_BACK_L, 102, 0, 0);
            setLedColor(LED_RIGHT_BACK_L, 102, 0, 0);
        }
    }
}