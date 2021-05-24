int offset_minutes, waiting_minutes, working_minutes;
int freq_hours, freq_minutes, duration_minutes;
int seconds_passed, minutes_passed;

int current_state;
#define IDLE_STATE 0
#define OFFSET_WAITING 1
#define WAITING 2
#define WORKING 3

#define RED_PIN PD7
#define GREEN_PIN PD6

void switch_to_idle()
{
  minutes_passed = 0;
  PORTD &= ~(1 << RED_PIN);
  PORTD &= ~(1 << GREEN_PIN);
  current_state = IDLE_STATE;
}

void switch_to_offset_waiting()
{
  if (!offset_minutes) {
    switch_to_waiting();
    return;
  }
  minutes_passed = 0;
  PORTD |= (1 << RED_PIN);
  PORTD |= (1 << GREEN_PIN);
  current_state = OFFSET_WAITING;
}

void switch_to_waiting()
{
  minutes_passed = 0;
  PORTD |= (1 << RED_PIN);
  PORTD &= ~(1 << GREEN_PIN);
  current_state = WAITING;
}

void switch_to_working()
{
  minutes_passed = 0;
  PORTD |= (1 << GREEN_PIN);
  PORTD &= ~(1 << RED_PIN);
  current_state = WORKING;
}

ISR(TIMER1_COMPA_vect) {
  // blink led
  seconds_passed = (seconds_passed+1) % 5;
  Serial.println(seconds_passed);
  Serial.flush();

  if (!seconds_passed) {
    minutes_passed++;
    switch (current_state)
    {
      case OFFSET_WAITING:
            if (minutes_passed == offset_minutes) {
              switch_to_waiting();
              Serial.println("switch to wait");
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
              Serial.println("switch to wait");
              Serial.flush();
            }
            break;
    }
  }
}

// Receive info as HH-MM-MMM
void receive_schedule()
{
  if (Serial.available()) {
    char buf[10];
    Serial.readString().toCharArray(buf, 10);
    delay(1000);
    freq_hours = (buf[0]-'0')*10+buf[1]-'0';
    freq_minutes = (buf[3]-'0')*10+buf[4]-'0';
    duration_minutes = (buf[6]-'0')*100+(buf[7]-'0')*10+buf[8]-'0';

    Serial.println(freq_hours);
    Serial.flush();
    Serial.println(freq_minutes);
    Serial.flush();
    Serial.println(duration_minutes);
    Serial.flush();
    Serial.println("Schedule set.");
    Serial.flush();
  }
}

void start_timer1()
{
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  OCR1A = 15624;                          // compare match register 16MHz/1024/1Hz-1
  TCCR1B = (1 << WGM12);                  // CTC mode
  TCCR1B |= (1 << CS12) | (1 << CS10);    // 1024 prescaler
  TIMSK1 |= (1 << OCIE1A);                // enable timer compare interrupt

  seconds_passed = 0;
  switch_to_offset_waiting();
}

void stop_timer1()
{
  TCCR1B = 0;
  switch_to_idle();
}

void setup()
{
  cli();
  Serial.begin(9600);
  Serial.println("in function setup");
  DDRD = (1 << RED_PIN) | (1 << GREEN_PIN);
  PORTD = 0;

  switch_to_idle();
  offset_minutes = 0;
  waiting_minutes = 2;
  working_minutes = 2;
  
  start_timer1();
   
  sei();
}


void loop()
{
  
  //receive_schedule();
  
}
