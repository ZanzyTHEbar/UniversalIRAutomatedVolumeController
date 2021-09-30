#include <IRremote.h>
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
int deadband;
int react;
int audio;
byte audiocounter = 0;
byte loudercounter = 0;
byte volcounter = 0;
byte timercounter = 0;
byte timercountervol = 0;
byte silencecounter = 0;
bool trig = 0;
bool lowertrig = 0;
bool lowertrigtimer = 0;
bool volup = 0;
byte i = 0;


void setup() {
  Serial.begin(250000);
  sbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  cbi(ADCSRA, ADPS0);
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1B |= (1 << CS12) | (1 << WGM12);
  TIMSK1 |= (1 << OCIE1A);
  OCR1A = 31250;
  TCCR2A = 0;
  TCCR2B = 0;
  TCCR2A |= (1 << WGM21);
  TCCR2B |= (1 << CS20) | (1 << CS21) | (1 << CS22) ;
  TIMSK2 |= (1 << OCIE2A);
  IrSender.begin(9, DISABLE_LED_FEEDBACK); //IR LED
  pinMode(8, OUTPUT); //Status
  digitalWrite(8, LOW);
  pinMode(A1, INPUT); //Audio
  pinMode(A2, INPUT); //Deadband
  pinMode(A3, INPUT); //React
  pinMode(A7, INPUT); //undervoltage
  deadband = map(analogRead(A2), 0, 1023, 511, 0);
  OCR2A = map(analogRead(A3), 1023, 0, 255, 127);
}

void loop() {

  Serial.print(0);
  Serial.print(" ");
  Serial.print(1023);
  Serial.print(" ");
  Serial.print(analogRead(A1));
  Serial.print(" ");
  Serial.print(511 + deadband);
  Serial.print(" ");
  Serial.println(511 - deadband);

  if (audiocounter > 10) {
    lowertrig = 1;
    audiocounter = 0;
  }

  if (loudercounter > 20) {
    while (i < volcounter) {
      IrSender.sendSAMSUNG(0x3434E817, 32); // Vol+
      i++;
      delay(200);
    }
    volup = 0;
    i = 0;
    volcounter = 0;
    loudercounter = 0;
  }

  audio = analogRead(A1);

  if ((volup == 1) && (audio < 540) && (audio > 480) && (trig == 0)) {
    TCNT2 = 0;
    trig = 1;
  }

  if ((volup == 1) && ((audio > 540) || (audio < 480))){
    loudercounter = 0;
  }

  if ((audio > (511 + deadband)) || (audio < (511 - deadband))) {
    if ((trig == 0) && (lowertrig == 0) && (volup == 0)) {
      TCNT2 = 0;
      trig = 1;
    }
    if (lowertrig == 1) {
      volcounter++;
      IrSender.sendSAMSUNG(0x34346897, 32); //Vol-
      delay(200);
      lowertrigtimer = 1;
    }
  }
}

ISR(TIMER1_COMPA_vect) {
  deadband = map(analogRead(A2), 0, 1023, 511, 0);
  OCR2A = map(analogRead(A3), 1023, 0, 255, 127); //200ms - 100ms
  if (analogRead(A7) < 540) {
    digitalWrite(8, HIGH);
  }
}

ISR(TIMER2_COMPA_vect) {
  if (trig == 1) {
    timercounter++;
    silencecounter = 0;
  }
  if ((timercounter > 6) && (volup == 0)) {
    audiocounter++;
    timercounter = 0;
    trig = 0;
  }

  if ((timercounter > 6) && (volup == 1)) {
    loudercounter++;
    timercounter = 0;
    trig = 0;
  }

  if ((audiocounter > 0) && (trig == 0) ) {
    silencecounter++;
    if (silencecounter > 50) {
      audiocounter = 0;
      silencecounter = 0;
    }
  }
  if (lowertrig == 1) {
    if (lowertrigtimer == 0) {
      timercountervol++;
    }
    if (lowertrigtimer == 1) {
      timercountervol = 0;
      lowertrigtimer = 0;
    }
    if (timercountervol > 100) {
      volup = 1;
      lowertrig = 0;
    }
  }
}
