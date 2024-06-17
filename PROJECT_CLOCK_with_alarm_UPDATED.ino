#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// Pin Definitions
#define MODE_BUTTON 2
#define HOUR_BUTTON 3
#define MINUTE_BUTTON 4
#define START_STOP_BUTTON 5  // Used for both timer and alarm start/stop
#define RESET_BUTTON 6       // Now used for both alarm reset and A.M./P.M.
#define BUZZER_PIN 8
#define DHTPIN 7  // Define the pin for DHT sensor

// DHT settings
#define DHTTYPE DHT11   // Define the type of DHT sensor

DHT dht(DHTPIN, DHTTYPE);

// Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C   // Most common I2C address for SSD1306

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RTC_DS3231 rtc;

enum Mode { CLOCK, TIMER, ALARM, TEMP_HUMIDITY };
Mode currentMode = CLOCK;

int hours = 0;
int minutes = 0;
int seconds = 0;

int alarmHours = 0;
int alarmMinutes = 0;
bool alarmRunning = false;

bool timerRunning = false;
bool timerFinished = false;
unsigned long lastMillis = 0;
unsigned long buzzerMillis = 0;
bool buzzerState = false;

const char* dayNames[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char* modeNames[] = {"Clock", "Timer", "Alarm", "Temp/Humid"};

void setup() {
  pinMode(MODE_BUTTON, INPUT_PULLUP);
  pinMode(HOUR_BUTTON, INPUT_PULLUP);
  pinMode(MINUTE_BUTTON, INPUT_PULLUP);
  pinMode(START_STOP_BUTTON, INPUT_PULLUP);
  pinMode(RESET_BUTTON, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  dht.begin();

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  // display.display();
  delay(2000);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void loop() {
  handleButtons();

  // Check for alarm
  checkAlarm();

  // Update timer if it's running
  if (timerRunning && !timerFinished) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastMillis >= 1000) {
      lastMillis = currentMillis;
      if (seconds == 0) {
        if (minutes == 0) {
          if (hours == 0) {
            timerRunning = false;
            timerFinished = true;
            // Start the buzzer when the timer is finished
            startBuzzer();
          } else {
            hours--;
            minutes = 59;
            seconds = 59;
          }
        } else {
          minutes--;
          seconds = 59;
        }
      } else {
        seconds--;
      }
    }
  }

  // Continue buzzer cycle if timer finished
  if (timerFinished) {
    startBuzzer();
  }

  switch (currentMode) {
    case CLOCK:
      displayClock();
      break;
    case TIMER:
      displayTimer();
      break;
    case ALARM:
      displayAlarm();
      break;
    case TEMP_HUMIDITY:
      displayTempHumidity();
      break;
  }
  display.display();
  delay(20);
}

void handleButtons() {
  if (digitalRead(MODE_BUTTON) == LOW) {
    currentMode = static_cast<Mode>((currentMode + 1) % 4);
    delay(200);
  }

  if (currentMode == TIMER) {
    if (digitalRead(START_STOP_BUTTON) == LOW) {
      timerRunning = !timerRunning;
      delay(100);
    }

    if (digitalRead(HOUR_BUTTON) == LOW) {
      hours = (hours + 1) % 24;
      delay(150);
    }

    if (digitalRead(MINUTE_BUTTON) == LOW) {
      minutes = (minutes + 1) % 60;
      delay(20);
    }
  }

  if (currentMode == ALARM) {
    if (digitalRead(HOUR_BUTTON) == LOW) {
      alarmHours = (alarmHours + 1) % 24;
      delay(150);
    }

    if (digitalRead(MINUTE_BUTTON) == LOW) {
      alarmMinutes = (alarmMinutes + 1) % 60;
      delay(100);
    }

    if (digitalRead(RESET_BUTTON) == LOW) {
      alarmRunning = false;
      digitalWrite(BUZZER_PIN, LOW);
      hours = 0;
      minutes = 0;
      seconds = 0;
      timerFinished = false;
      delay(20);
    }

    if (digitalRead(START_STOP_BUTTON) == LOW) {
      alarmRunning = !alarmRunning;
      delay(150);
    }
  }

  if (digitalRead(RESET_BUTTON) == LOW) {
    timerRunning = false;
    timerFinished = false;
    alarmRunning = false;
    digitalWrite(BUZZER_PIN, LOW);
    hours = 0;
    minutes = 0;
    seconds = 0;
    delay(20);
  }
}

void checkAlarm() {
  if (alarmRunning && currentMode != ALARM) {
    DateTime now = rtc.now();
    if (now.hour() == alarmHours && now.minute() == alarmMinutes) {
      startBuzzer();
    } else {
      digitalWrite(BUZZER_PIN, LOW);
    }
  }
}

void startBuzzer() {
  unsigned long currentMillis = millis();
  if (currentMillis - buzzerMillis >= 150) {
    buzzerMillis = currentMillis;
    buzzerState = !buzzerState;
    digitalWrite(BUZZER_PIN, buzzerState);
  }
}
   // THIS IS FOR THE CLOCK MODE
void displayClock() {
  DateTime now = rtc.now();
  display.clearDisplay();

  // DISPLAY THE MODE NAME FOR THE "CLOCK"
  display.setCursor(27, 0);
  display.setTextSize(2); // Larger text size for the mode name
  display.print(modeNames[currentMode]);

  // DISPLAY THE 24-FORMAT TIME HOUR FOR THE CLOCK SECTION
  display.setCursor(11, 20);
  display.print(now.hour());
  display.print(':');
  display.print(now.minute() < 10 ? "0" : "");
  display.print(now.minute());
  display.print(':');
  display.print(now.second() < 10 ? "0" : "");
  display.print(now.second());

  // DISPLAYY THE DATE AND DAY TIME FOR THE CLOCK MODE
  display.setTextSize(1); // Smaller text size for the date
  display.setCursor(21, 40);
  display.print(dayNames[now.dayOfTheWeek()]);
  display.print(' ');
  display.print(now.day());
  display.print('/');
  display.print(now.month());
  display.print('/');
  display.print(now.year());
  
}

void displayTimer() {
  display.clearDisplay();

  // Display mode name for "TIMER"
  display.setCursor(27, 0);
  display.setTextSize(2); // Larger text size for the mode name
  display.print(modeNames[currentMode]);

  // Display timer
  display.setCursor(19, 20);
  display.print(hours);
  display.print(':');
  display.print(minutes < 10 ? "0" : "");
  display.print(minutes);
  display.print(':');
  display.print(seconds < 10 ? "0" : "");
  display.print(seconds);

  // DISPLAY MODE FOR THE "TIMER" SECTION.
  display.setTextSize(2); // Smaller text size for the message
  display.setCursor(5, 43);
  if (timerRunning) {
    display.print("TO GO");
  } else if (timerFinished) {
    display.print("TIME TO GO");
  }
}
  // THIS IS THE CODE FOR THE "ALARM" SECTION.
void displayAlarm() {
  display.clearDisplay();

  // DISPLAY MODE NAME FOR ALARM SECTION.
  display.setCursor(27, 0);
  display.setTextSize(2); // Larger text size for the mode name
  display.print(modeNames[currentMode]);

  // Display alarm time in 24-hour format
  display.setCursor(35, 20);
  display.print(alarmHours);
  display.print(':');
  display.print(alarmMinutes < 10 ? "0" : "");
  display.print(alarmMinutes);

  // Display message FOR ALARM STATUS
  display.setTextSize(2); // Smaller text size for the message
  display.setCursor(9, 45);
  if (alarmRunning) {
    display.print("ALARM SET");
  } else if (currentMode != ALARM) {
    display.print("TIME TO GO");
  }

  // Check if alarm should sound
  DateTime now = rtc.now();
  if (alarmRunning) {
    if (now.hour() == alarmHours && now.minute() == alarmMinutes) {
      startBuzzer();
    } else {
      digitalWrite(BUZZER_PIN, LOW);
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void displayTempHumidity() {
  display.clearDisplay();

  // Display mode name
  display.setCursor(27, 0);
  display.setTextSize(1); // Larger text size for the mode name
  display.print(modeNames[currentMode]);

  // Read temperature and humidity
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Check if readings are valid
  if (isnan(temperature) || isnan(humidity)) {
    display.setCursor(0, 20);
    display.setTextSize(1);
    display.print("Failed to read");
    display.setCursor(0, 30);
    display.print("from DHT sensor!");
  } else {
    display.setCursor(0, 20);
    display.setTextSize(2);
    display.print("T: ");
    display.print(temperature);
    display.print(" C");

    display.setCursor(0, 40);
    display.print("H: ");
    display.print(humidity);
    display.print(" %");
  }
}
