int PhotoresistorPin = A0;
int Green = D1;
int Blue = D2;
int Vo;

class Led{
  private:
    int pin;
    unsigned long last_click_time = 0;
    bool is_pin_on = false;
  
  public:
    Led(int input_pin) {
      pin = input_pin;
      pinMode(pin, OUTPUT);
      digitalWrite(pin, LOW);

      //Set boolean variable to false
      is_pin_on = false;
      last_click_time = millis();
    }
  
  void flash(unsigned long on_time, unsigned long off_time) {
    unsigned long current_time = millis();
      //if statement to check if motor is running
      if (is_pin_on) {
        if (current_time - last_click_time >= on_time) {
          digitalWrite(pin, LOW);
          last_click_time = current_time;
          is_pin_on = false;
        }
      } else {
        if (current_time - last_click_time >= off_time) {
          digitalWrite(pin, HIGH);
          last_click_time = current_time;
          is_pin_on = true;
        }
      }
  }
};

Led greenLed(Green);
Led blueLed(Blue);

void setup() {
  Serial.begin(9600);       
  pinMode(A0, INPUT);    
}

void loop() {
  Vo = analogRead(PhotoresistorPin);
  
  if (Vo < 1000) {
    blueLed.flash(500, 500);
  } else {
    greenLed.flash(500, 500);
  }
  Serial.print("Voltage: "); 
  Serial.println(Vo);
  delay(500);
}