unsigned int offset_minutes, waiting_minutes, working_minutes;
unsigned int seconds_passed, minutes_passed;

int current_state;
#define IDLE_STATE 0
#define OFFSET_WAITING 1
#define WAITING 2
#define WORKING 3
#define WORKING_ONCE 4

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
  PORTB |= (1 << START_PIN);
  current_state = WORKING;
}

void switch_to_working_once()
{
  minutes_passed = 0;
  PORTD |= (1 << GREEN_PIN);
  PORTD &= ~(1 << RED_PIN);
  PORTB |= (1 << START_PIN);
  current_state = WORKING_ONCE;
}

ISR(TIMER1_COMPA_vect)
{
  seconds_passed = (seconds_passed+1) % TIME_MULTIPLIER;
  Serial.println(seconds_passed);
  Serial.flush();
  PORTB &= ~(1 << START_PIN);

  if (!seconds_passed) {
    minutes_passed++;
    switch (current_state)
    {
      case OFFSET_WAITING:
            if (minutes_passed == offset_minutes) {
              switch_to_working();
              Serial.println("switch to work");
              Serial.flush();
            }
            break;

      case WAITING:
            if (minutes_passed == waiting_minutes) {
              switch_to_working();
              Serial.println("switch to work");
              Serial.flush();
            }
            break;

      case WORKING:
            if (minutes_passed == working_minutes) {
              switch_to_waiting();
              PORTB |= (1 << STOP_PIN);
              Serial.println("switch to wait");
              Serial.flush();
            }
            break;
      case WORKING_ONCE:
            if (minutes_passed == WORK_ONCE_MINUTES) {
              stop_timer1();
              Serial.println("stop timer1");
              Serial.flush();
            }
            break;
    }
  }
}

void start_timer1(int once)
{
  seconds_passed = 0;
  minutes_passed = 0;

  if (once)
    switch_to_working_once();
  else if (offset_minutes)
    switch_to_offset_waiting();
  else
    switch_to_waiting();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  OCR1A = 15624;                          // compare match register 16MHz/1024/1Hz-1
  TIMSK1 |= (1 << OCIE1A);                // enable timer compare interrupt
  TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10); // CTC, 1024 prescaler
}

void stop_timer1()
{
  TCCR1B = 0;
  PORTB |= (1 << STOP_PIN);
  switch_to_idle();
}

bool is_number(char c)
{
  return c >= '0' && c <= '9';
}

// Receive info as HHH-HHH:MMM-MMM
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

    if (s.startsWith("info")) {
      Serial.println("Scrieti o comanda de tipul [HH]H-[HH]H:[MM]M-[MM]M pentru a seta intarzierea pentru pornire, timpul de asteptare si cel de irigare.");
      Serial.flush();
      return;
    }
    
      
    int i = 0, last_i = 0;
    unsigned int data[4] = {0};

    for (int idx = 0; idx < 4; idx++) {
      while (i < s.length() && is_number(s.c_str()[i])) {
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

    if (data[1] + data[2] == 0 || data[3] == 0) {
      Serial.println("Perioada de asteptare/lucru nu poate fi 0!");
      Serial.flush();
      return;
    }

    if (data[1] * TIME_MULTIPLIER + data[2] <= data[3]) {
      Serial.println("Perioada de asteptare trebuie sa fie strict mai mare decat cea de lucru!");
      Serial.flush();
      return;
    }
    
    stop_timer1();
    offset_minutes = data[0] * TIME_MULTIPLIER;
    waiting_minutes = data[1] * TIME_MULTIPLIER + data[2];
    working_minutes = data[3];
    
    Serial.println(String("offset=") + offset_minutes);
    Serial.flush();
    Serial.println(String("wait=") + waiting_minutes);
    Serial.flush();
    Serial.println(String("work=") + working_minutes);
    Serial.flush();
    Serial.println("Programare reusita.");
    Serial.flush();

    start_timer1(0);
  }
}

void setup_button_interrupts()
{
  EICRA = (1 << ISC11) | (1 << ISC10);  // INT1 on rising edge
  EICRA |= (1 << ISC01) | (1 << ISC00); // INT1 on rising edge
  EIMSK = (1 << INT0) | (1 << INT1);    // Activate interrupts
}

ISR(INT0_vect)
{
  Serial.println("Stop");
  Serial.flush();
  stop_timer1();
}

ISR(INT1_vect)
{
  Serial.println("Start");
  Serial.flush();

  offset_minutes = 0;
  waiting_minutes = 0;
  working_minutes = 0;
  start_timer1(1);
}

void setup()
{
  cli();
  Serial.begin(9600);
  Serial.println("in function setup");
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
  delay(500);
  PORTB &= ~(1 << START_PIN);
  PORTB &= ~(1 << STOP_PIN);
  receive_schedule();
}
