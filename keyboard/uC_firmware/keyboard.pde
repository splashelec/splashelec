/*
  Keyboard for the autopilot
  
  Bernt Weber 2010
  
 */
 

#define txPin 1
#define buzzerPin 3
#define LEDStby 5
#define LEDAuto 6
#define LEDJoys 7

#define S 14
#define A 15
#define J 16
#define M1   17
#define P1   18
#define M10  19
#define P10  8

// inteval (ms) between two button scans
#define buttonInterval 100

// intevalls before interpreting buttons 
// to make shure that we don't have a two button action
#define buttonDelay 2

// .If button kept pressed, repeat action every buttonRepeat intervals.
#define buttonRepeat 5


// Button states
//   xxx[0] is the newest state.
byte S_state[buttonDelay],  A_state[buttonDelay],   J_state[buttonDelay],
     M1_state[buttonDelay], M10_state[buttonDelay], M90_state[buttonDelay],
     P1_state[buttonDelay], P10_state[buttonDelay], P90_state[buttonDelay]; 

// Action interval counters
int S_count=0, A_count=0, J_count=0,
    M1_count=0, M10_count=0, M90_count=0, 
    P1_count=0, P10_count=0, P90_count=0;


void initState(byte buttonState[]){

  int i;
  for (i = 0; i < buttonDelay; i++)
     buttonState[i] = 0;
}

void ageState(byte buttonState[]){

  int i;
  for (i = buttonDelay - 2; i >= 0; i--)
     buttonState[i + 1] = buttonState[i];
     
  buttonState[0] = 0;
}

void readButton(byte button, byte state[]) {
    // reads button and updates state
    ageState(state);
    
    if (digitalRead(button)){
      state[0] = 1;
    } else {
      state[0] = 0;
    };
  }


void beep(){
  pinMode(buzzerPin, OUTPUT);
  delay(100);
  pinMode(buzzerPin, INPUT);

}

void beepOn(){
// switches the buzzer on //
  pinMode(buzzerPin, OUTPUT);
}

void beepOff(){
// switches the buzzer off //
  pinMode(buzzerPin, INPUT);
}

void setup() {                
  // initialize the digital pin as an output.
  // Pin 13 has an LED connected on most Arduino boards:
  pinMode(13, OUTPUT); 

  pinMode(txPin, OUTPUT);  
  Serial.begin(38400);
 
  //Timer/Counter2
  TCCR2A = _BV(COM2A0) | _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(WGM22) | _BV(CS21)| _BV(CS20); // toggle OC2A on OCR2A match; prescaler: /32
  OCR2A = 124;
  OCR2B = 62;
  //    * Output A frequency: 16 MHz / 32 / (124+1) / 2 = 2 kHz
  //    * Output A duty cycle: 50%
  //    * Output B frequency: 16 MHz / 8 / (124+1) = 4 kHz
  //    * Output B duty cycle: (39+1) / (124+1) = 40 % 
  //    Output B = DigitalOut 3;
  //    Output A = DigitalOut 11;
  
  pinMode(LEDStby, OUTPUT);
  pinMode(LEDAuto, OUTPUT);
  pinMode(LEDJoys, OUTPUT);

  initState(S_state);
  initState(A_state);
  initState(J_state);
  initState(M1_state);
  initState(M10_state);
  initState(M90_state);
  initState(P1_state);
  initState(P10_state);
  initState(P90_state);

  // buzzer
  beep();
  delay(050);
  beep();  
  digitalWrite(LEDStby, 1);
  delay(200);
  digitalWrite(LEDStby, 0);
  digitalWrite(LEDAuto, 1);  
  delay(200);
  digitalWrite(LEDAuto, 0);  
  digitalWrite(LEDJoys, 1);  
  delay(200);
  digitalWrite(LEDJoys, 0);
  digitalWrite(LEDStby, 1);
  Serial.print('K');  // keyboard present
  Serial.print('S');  // Standby
}

void loop() {
    
  readButton(S, S_state);
  readButton(A, A_state);
  readButton(J, J_state);
  readButton(M1, M1_state);
  readButton(M10, M10_state);
  readButton(P1, P1_state);
  readButton(P10, P10_state);
  
  ageState(M90_state);
  // detect virtual M90 button
  if ((M1_state[0] && M10_state[0]) || 
      ((M1_state[0] || M10_state[0]) && M90_state[1])) {
    M90_state[0] = 1; 
    // clear the composing button states
    initState(M1_state); initState(M10_state);
  } else M90_state[0] = 0;
  
  ageState(P90_state);
  // detect virtual P90 button
  if ((P1_state[0] && P10_state[0]) || 
      ((P1_state[0] || P10_state[0]) && P90_state[1])) {
    P90_state[0] = 1; 
    // clear the composing button states
    initState(P1_state); initState(P10_state);
  } else P90_state[0] = 0;
  
  if (S_state [buttonDelay-1])  S_count   += 1; else S_count   = 0;
  if (A_state[buttonDelay-1])   A_count   += 1; else A_count   = 0;
  if (J_state[buttonDelay-1])   J_count   += 1; else J_count   = 0;
  if (M1_state [buttonDelay-1]) M1_count  += 1; else M1_count  = 0;
  if (M10_state[buttonDelay-1]) M10_count += 1; else M10_count = 0;
  if (M90_state[buttonDelay-1]) M90_count += 1; else M90_count = 0;
  if (P1_state [buttonDelay-1]) P1_count  += 1; else P1_count  = 0;
  if (P10_state[buttonDelay-1]) P10_count += 1; else P10_count = 0;
  if (P90_state[buttonDelay-1]) P90_count += 1; else P90_count = 0;
  
  if (S_count == 1) // no repeat for this action
  {
    Serial.print('S'); // transmit command
    digitalWrite(LEDStby, 1);
    digitalWrite(LEDAuto, 0);
    digitalWrite(LEDJoys, 0);    
    beepOn();
  };
  if (A_count == 1) // no repeat for this action
  {
    Serial.print('A'); // transmit command
    digitalWrite(LEDStby, 0);
    digitalWrite(LEDAuto, 1);
    digitalWrite(LEDJoys, 0);    
    beepOn();
  };
  if (J_count == 1) // no repeat for this action
  {
    Serial.print('J'); // transmit command
    digitalWrite(LEDStby, 0);
    digitalWrite(LEDAuto, 0);
    digitalWrite(LEDJoys, 1);    
    beepOn();
  };
  if (((M1_count - 1) % buttonRepeat) == 0) // repeat action every buttonRepeat intervals
  {
    Serial.print('7'); // transmit command
    beepOn();
  };
  if (((M10_count - 1) % buttonRepeat) == 0) // repeat action every buttonRepeat intervals
  {
    Serial.print('4'); // transmit command
    beepOn();
  };
  if (M90_count == 1) // no repeat for this action
  {
    Serial.print('1'); // transmit command
    beepOn();
  };
  if (((P1_count - 1) % buttonRepeat) == 0) // repeat action every buttonRepeat intervals
  {
    Serial.print('9'); // transmit command
    beepOn();
  };
  if (((P10_count - 1) % buttonRepeat) == 0) // repeat action every buttonRepeat intervals
  {
    Serial.print('6'); // transmit command
    beepOn();
  };
  if (P90_count == 1) // no repeat for this action
  {
    Serial.print('3'); // transmit command
    beepOn();
  };
  
  
  delay(buttonInterval / 2);
  beepOff();
  delay(buttonInterval / 2);
  
  Serial.print('K'); // keyboard present
}
