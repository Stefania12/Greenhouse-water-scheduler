int freq_hours, freq_minutes, duration_minutes;
void setup()
{
  Serial.begin(9600);
  Serial.println("in function setup");
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

void loop()
{
  //receive_schedule();
  
}
