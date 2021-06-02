unsigned int offset_minutes, waiting_minutes, working_minutes;
unsigned int seconds_passed, minutes_passed;
unsigned long stop_impulse;

int current_state;
#define IDLE_STATE 0
#define OFFSET_WAITING 1
#define WAITING 2
#define WORKING 3

#define RED_PIN PD7
#define GREEN_PIN PD6

#define START_PIN PB0
#define STOP_PIN PB1

#define WORK_ONCE_MINUTES 2

#define TIME_MULTIPLIER 5

void switch_to_idle()
{
  minutes_passed = 0;
  PORTD &= ~(1 << RED_PIN);
  PORTD &= ~(1 << GREEN_PIN);
  current_state = IDLE_STATE;
}

void switch_to_offset_waiting()
{
  minutes_passed = 0;
  PORTD |= (1 << RED_PIN);
  PORTD |= (1 << GREEN_PIN);
  current_state = OFFSET_WAITING;
}

void switch_to_waiting()
{
  PORTD |= (1 << RED_PIN);
  PORTD &= ~(1 << GREEN_PIN);
  current_state = WAITING;
}

void switch_to_working()
{
  minutes_passed = 0;
  PORTD |= (1 << GREEN_PIN);
  PORTD &= ~(1 << RED_PIN);
  PORTB = (1 << START_PIN);
  current_state = WORKING;
}

ISR(TIMER1_COMPA_vect)
{
  seconds_passed = (seconds_passed+1) % TIME_MULTIPLIER;
  PORTB = 0;

  if (!seconds_passed) {
    minutes_passed++;
    switch (current_state)
    {
      case OFFSET_WAITING:
            if (minutes_passed == offset_minutes)
              switch_to_working();
            break;

      case WAITING:
            if (minutes_passed == waiting_minutes)
              switch_to_working();
            break;

      case WORKING:
            if (minutes_passed == working_minutes) {
              if (waiting_minutes == 0) {
                stop_timer1();
              } else {
                switch_to_waiting();
                PORTB = (1 << STOP_PIN);
              }
            }
            break;
    }
  }
}

void start_timer1()
{
  seconds_passed = 0;
  minutes_passed = 0;

  if (offset_minutes > 0)
    switch_to_offset_waiting();
  else
    switch_to_working();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  OCR1A = 15624;  // compare match register 16MHz/1024/1Hz-1
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
  TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);  // CTC, 1024 prescaler
}

void stop_timer1()
{
  TCCR1B = 0;
  PORTB &= ~(1 << START_PIN);
  PORTB |= (1 << STOP_PIN);
  switch_to_idle();
  stop_impulse = millis();
}

bool is_digit(char c)
{
  return c >= '0' && c <= '9';
}

void receive_schedule()
{
  if (Serial.available()) {
    String s;
    s = Serial.readString();
    delay(1000);
    if (s.length() > 16) {
      Serial.println("Mesaj prea lung!");
      Serial.flush();
      return;
    }
    s.toLowerCase();
    if (s.startsWith("stop")) {
      stop_timer1();
      Serial.println("Programare oprita.");
      Serial.flush();
      return;
    }

    if (s.startsWith("ajutor")) {
      Serial.println("Scrieti o comanda de tipul [HH]H-[HH]H:[MM]M-[MM]M pentru a seta: intarziere_pornire-timp_asteptare-timp_irigare.");
      Serial.flush();
      return;
    }

    int i = 0, last_i = 0;
    unsigned int data[4] = {0}, offset, wait, work;

    for (int idx = 0; idx < 4; idx++) {
      while (i < s.length() && is_digit(s.c_str()[i])) {
        data[idx] = data[idx]*10 + s.c_str()[i]-'0';
        i++;
      }
      if (i - last_i > 3) {
        Serial.println("Prea multe cifre pentru date!");
        Serial.flush();
        return;
      }

      if (i == last_i) {
        Serial.println("Prea putine cifre pentru date!");
        Serial.flush();
        return;
      }
      
      i++;
      last_i = i;
    }

    offset = data[0] * TIME_MULTIPLIER;
    wait = data[1] * TIME_MULTIPLIER + data[2];
    work = data[3];

    if (work == 0) {
      Serial.println("Perioada de lucru nu poate fi 0!");
      Serial.flush();
      return;
    }

    if (wait > 0 && wait <= work) {
      Serial.println("Perioada de asteptare trebuie sa fie strict mai mare decat cea de lucru!");
      Serial.flush();
      return;
    }
    
    stop_timer1();
    offset_minutes = offset;
    waiting_minutes = wait;
    working_minutes = work;
    
    Serial.println("Programare reusita.");
    Serial.flush();

    start_timer1();
  }
}

void setup_button_interrupts()
{
  EICRA = (1 << ISC11) | (1 << ISC10);  // INT1 on rising edge
  EICRA |= (1 << ISC01) | (1 << ISC00); // INT0 on rising edge
  EIMSK = (1 << INT0) | (1 << INT1);    // Activate interrupts
}

ISR(INT0_vect)
{
  stop_timer1();
}

ISR(INT1_vect)
{
  PORTB = 0;
  offset_minutes = 0;
  waiting_minutes = 0;
  working_minutes = WORK_ONCE_MINUTES;
  start_timer1();
}

void setup()
{
  cli();
  Serial.begin(9600);
  DDRD = (1 << RED_PIN) | (1 << GREEN_PIN);
  DDRB = (1 << START_PIN) | (1 << STOP_PIN);
  PORTD = 0;

  switch_to_idle();
  offset_minutes = 0;
  waiting_minutes = 0;
  working_minutes = 0;

  setup_button_interrupts();
  
  sei();
}

void loop()
{
  if (stop_impulse && abs(millis() - stop_impulse) >= 1000) {
    PORTB = 0;
    stop_impulse = 0;
  }

  receive_schedule();
}
