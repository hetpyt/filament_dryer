#include <timer-api.h>
#include <Thermistor.h>
#include <NTC_Thermistor.h>
#include <TM1637Display.h>

#define DEBUG
#define MAX_TARGET_TEMP 70.0
#define MAIN_LOOP_DELAY 100
// menu
#define MENU_DELAY  20  
// heather thermistor config HT1
#define HT1_PIN             A1
#define HT1_REFERENCE_RESISTANCE   10000
#define HT1_NOMINAL_RESISTANCE     100000
#define HT1_NOMINAL_TEMPERATURE    25
#define HT1_B_VALUE                3950
#define HT1_MAX_TEMP               100.0
// heather 
#define PWN_PIN 6
#define MAX_PWM 200 // power limit
// fan
#define FAN_PWM_PIN 9
// display
#define DSP_CLK_PIN 7
#define DSP_DIO_PIN 8
#define DSP_SYM_DEG (SEG_A | SEG_B | SEG_F | SEG_G)
// encoder
#define ENC_A 2       // encoder pin A, int
#define ENC_B 4       // encoder pin B
#define ENC_TYPE 1    // encoder type, 0 or 1

const uint8_t SEG_DEGREE[] = {
  SEG_A | SEG_B | SEG_F | SEG_G           // degree symbol
  };
//
volatile double _ht1_target = 40.0;
volatile double _ht1_temp = 0.0;
volatile boolean _enc_state0, _enc_lastState, _enc_turnFlag;

uint8_t _dsp_buff[4] = {0,0,0,0};
uint8_t _menuPage = 0;
uint16_t _menuDelay = 0;

Thermistor* _ht1 = new NTC_Thermistor(HT1_PIN, HT1_REFERENCE_RESISTANCE, HT1_NOMINAL_RESISTANCE, HT1_NOMINAL_TEMPERATURE, HT1_B_VALUE);

TM1637Display _dsp(DSP_CLK_PIN, DSP_DIO_PIN);

void set_buffer_temp(uint8_t sym, int temp) {
  _dsp_buff[0] = sym;
  _dsp_buff[1] = _dsp.encodeDigit();
  _dsp_buff[1] = _dsp.encodeDigit();
  
  _dsp_buff[3] = DSP_SYM_DEG;
    
}

void setup() {
  Serial.begin(9600);
  _dsp.setBrightness(3);
  timer_init_ISR_10Hz(TIMER_DEFAULT);
  attachInterrupt(0, int0, CHANGE);
}

void timer_handle_interrupts(int timer) {
  // get ht1 temp
  _ht1_temp = _ht1->readCelsius();
  // check illegal temp values
  if (_ht1_temp <= 0.0 || _ht1_temp >= HT1_MAX_TEMP) {
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

void int0() {
  _enc_state0 = bitRead(PIND, ENC_A);
  if (_enc_state0 != _enc_lastState) {
    _enc_turnFlag = !_enc_turnFlag;
    if (_enc_turnFlag) {
      _ht1_target += (bitRead(PIND, ENC_B) != _enc_lastState) ? -1 : 1;
      if (_ht1_target > MAX_TARGET_TEMP) _ht1_target = MAX_TARGET_TEMP;
      if (_ht1_target < 0) _ht1_target = 0;
      _menuPage = 1;
    }    
    _enc_lastState = _enc_state0;
  }
}


void loop() {
  // put your main code here, to run repeatedly:
  if (_menuPage == 0) {
    // show current temperature
    //_dsp.clear();
    _dsp.showNumberDec(_ht1_temp, false, 2, 1);
    _dsp.setSegments(SEG_DEGREE, 1, 3);
  }
  else if (_menuPage == 1) {
    // show target temperature
    //_dsp.clear();
    _dsp.showNumberDec(_ht1_target, false, 2, 0);
    _dsp.setSegments(SEG_DEGREE, 2, 2);
  }
  delay(MAIN_LOOP_DELAY);
  _menuDelay++;
  if (_menuDelay >= MENU_DELAY) {
    _menuDelay = 0;
    _menuPage = 0;
  }
}
