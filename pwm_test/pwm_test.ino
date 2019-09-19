/*
test comment
*/
#include <timer-api.h>
#include <Thermistor.h>
#include <NTC_Thermistor.h>
#include <TM1637Display.h>

//#define DEBUG
#define MAX_TARGET_TEMP 70
#define MAX_TIMER_HOURS 24
#define MAIN_LOOP_DELAY 100
// menu
#define MENU_DELAY  100 
#define LAST_MENU_PAGE  2
// heather thermistor config HT1
#define HT1_PIN             A1
#define HT1_REFERENCE_RESISTANCE   10000
#define HT1_NOMINAL_RESISTANCE     100000
#define HT1_NOMINAL_TEMPERATURE    25
#define HT1_B_VALUE                3950
#define HT1_MAX_TEMP               100
// heather 
#define PWN_PIN 6
#define MAX_PWM 200 // power limit
// fan
#define FAN_PWM_PIN 9
// display
#define DSP_CLK_PIN 7
#define DSP_DIO_PIN 8
#define DSP_SYM_DEG (SEG_A | SEG_B | SEG_F | SEG_G)
#define DSP_SYM_T   (SEG_D | SEG_E | SEG_F | SEG_G)
#define DSP_SYM_C   (SEG_D | SEG_E | SEG_G)
#define DSP_SYM_H   (SEG_C | SEG_E | SEG_F | SEG_G)


// encoder
#define ENC_A     2 // encoder pin A, int
#define ENC_B     4 // encoder pin B
#define ENC_PB    3 // encoder pushbutton
#define ENC_TYPE  1 // encoder type, 0 or 1
#define ENC_PB_MS 100 // delay in ms between pressing button  
const uint8_t SEG_DEGREE[] = {
  SEG_A | SEG_B | SEG_F | SEG_G           // degree symbol
  };
const uint8_t SEG_HOURS[] = {
  SEG_F | SEG_E | SEG_C | SEG_G           // h symbol
  };

//
volatile uint8_t _ht1_target = 40;
volatile uint8_t _ht1_temp = 0;
volatile boolean _enc_state0, _enc_lastState, _enc_turnFlag;
volatile uint8_t _enc_counter = 0;
volatile uint32_t _enc_pb_last_ms = 0; // last value of millis() for encoder pushbutton to avoid noise
volatile uint8_t _dsp_buff[4] = {0,0,0,0};
volatile uint8_t _menuPage = 0;
volatile uint16_t _menuDelay = 0;

volatile uint8_t _timer_hours = 1; // working time in hours to configure in menu
volatile uint16_t _working_counter_100ms = 0;

Thermistor* _ht1 = new NTC_Thermistor(HT1_PIN, HT1_REFERENCE_RESISTANCE, HT1_NOMINAL_RESISTANCE, HT1_NOMINAL_TEMPERATURE, HT1_B_VALUE);

TM1637Display _dsp(DSP_CLK_PIN, DSP_DIO_PIN);

void set_dsp_buffer(uint8_t sym0, uint8_t num, uint8_t sym3) {
  _dsp_buff[0] = sym0;
  _dsp_buff[2] = _dsp.encodeDigit(num % 10);
  _dsp_buff[1] = _dsp.encodeDigit(num / 10);
  _dsp_buff[3] = sym3;
}

void setup() {
  Serial.begin(9600);
  _dsp.setBrightness(3);
  timer_init_ISR_10Hz(TIMER_DEFAULT);
  attachInterrupt(digitalPinToInterrupt(ENC_A), int0, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_PB), int1, LOW);
}

void timer_handle_interrupts(int timer) {
  _working_counter_100ms++; 
  // get ht1 temp
  _ht1_temp = _ht1->readCelsius();
  // check illegal temp values
  if (_timer_hours == 0 || _ht1_temp <= 0 || _ht1_temp >= HT1_MAX_TEMP) {
    analogWrite(PWN_PIN, 0); // turn off heather
    return;
  }
  if (_ht1_temp >= _ht1_target) {
    analogWrite(PWN_PIN, 0); // turn off heather
    //analogWrite(FAN_PWM_PIN, 255); // turn on fan
  }
  else {
    analogWrite(PWN_PIN, MAX_PWM); // turn on heather
    //analogWrite(FAN_PWM_PIN, 0); //turn off fan
  }
#ifdef DEBUG
  Serial.println(_ht1_temp);
#endif
}
// interrupt handler for encoder rotation
void int0() {
  _enc_state0 = bitRead(PIND, ENC_A);
  if (_enc_state0 != _enc_lastState) {
    _enc_turnFlag = !_enc_turnFlag;
    if (_enc_turnFlag) {
       _enc_counter += (bitRead(PIND, ENC_B) != _enc_lastState) ? -1 : 1;
       _menuDelay = 0;
    }    
    _enc_lastState = _enc_state0;
  }
}
// interrupt handler for encoder button
void int1() {
  if (millis() - _enc_pb_last_ms < ENC_PB_MS)
    return;
  _menuPage++;
  if (_menuPage > LAST_MENU_PAGE)
    _menuPage = 0;
  _enc_pb_last_ms = millis();
  _menuDelay = 0;
}

void loop() {
  // put your main code here, to run repeatedly:
  if (_menuPage == 0) {
    // show current temperature
    //_dsp.clear();
    //_dsp.showNumberDec(_ht1_temp, false, 2, 1);
    //_dsp.setSegments(SEG_DEGREE, 1, 3);
    set_dsp_buffer(0, _ht1_temp, DSP_SYM_DEG);
  }
  else if (_menuPage == 1) {
    _ht1_target += _enc_counter;
    _enc_counter = 0;
    if (_ht1_target > MAX_TARGET_TEMP) _ht1_target = MAX_TARGET_TEMP;
    if (_ht1_target < 0) _ht1_target = 0;

    // show target temperature
    //_dsp.clear();
    //_dsp.showNumberDec(_ht1_target, false, 2, 0);
    //_dsp.setSegments(SEG_DEGREE, 2, 2);
    set_dsp_buffer(DSP_SYM_T, _ht1_target, DSP_SYM_DEG);
  }
  else if (_menuPage == 2) {
    _timer_hours += _enc_counter;
    _enc_counter = 0;
    if (_timer_hours > MAX_TIMER_HOURS) _timer_hours = MAX_TIMER_HOURS;
    if (_timer_hours < 0) _timer_hours = 0;
    // show working time
    //_dsp.showNumberDec(_timer_hours, false, 2, 0);
    //_dsp.setSegments(SEG_HOURS, 2, 2);
    set_dsp_buffer(DSP_SYM_T, _timer_hours, DSP_SYM_H);
  }
  // update display
  _dsp.setSegments(_dsp_buff);

  delay(MAIN_LOOP_DELAY);

  // check timer
  if (_working_counter_100ms >= 36000) {
    _working_counter_100ms = 0;
    _timer_hours--;
  }
  
  _menuDelay++;
  if (_menuDelay >= MENU_DELAY) {
    _menuDelay = 0;
    _menuPage = 0;
  }
}
