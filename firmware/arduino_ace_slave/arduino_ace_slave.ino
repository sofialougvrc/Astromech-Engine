#include <Servo.h>

constexpr unsigned long BAUD_RATE = 115200;
constexpr byte MAX_SERVOS = 6;
constexpr byte SERVO_PINS[MAX_SERVOS] = {9, 10, 11, 6, 5, 3};
constexpr int SERVO_OPEN_DEG = 100;
constexpr int SERVO_CLOSE_DEG = 15;
constexpr int SERVO_CENTER_DEG = 90;

Servo servos[MAX_SERVOS];
bool servoAttached[MAX_SERVOS] = {false};
String inputLine;

void setup() {
  Serial.begin(BAUD_RATE);
  inputLine.reserve(96);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("OK:ACE_SLAVE_READY");
}

void loop() {
  while (Serial.available() > 0) {
    char c = static_cast<char>(Serial.read());
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      handleLine(inputLine);
      inputLine = "";
      continue;
    }
    if (inputLine.length() < 95) {
      inputLine += c;
    } else {
      inputLine = "";
      Serial.println("ERR:LINE_TOO_LONG");
    }
  }
}

void handleLine(const String& line) {
  if (line.length() == 0) {
    return;
  }
  if (line == "PING") {
    Serial.println("OK:PONG");
    return;
  }
  if (line == "STATUS") {
    Serial.println("OK:ACE_SLAVE_READY");
    return;
  }

  int first = line.indexOf(':');
  int second = line.indexOf(':', first + 1);
  if (first < 0 || second < 0) {
    Serial.println("ERR:BAD_FRAME");
    return;
  }

  String type = line.substring(0, first);
  String channelText = line.substring(first + 1, second);
  String value = line.substring(second + 1);
  type.toUpperCase();
  value.toUpperCase();

  int channel = channelText.toInt();
  if (type == "SERVO") {
    handleServo(channel, value);
  } else if (type == "LIGHT") {
    handleLight(channel, value);
  } else if (type == "AUDIO") {
    Serial.println("OK:AUDIO_ACK");
  } else {
    Serial.println("ERR:UNKNOWN_TYPE");
  }
}

void handleServo(int channel, const String& value) {
  int index = channel - 1;
  if (index < 0 || index >= MAX_SERVOS) {
    Serial.println("ERR:SERVO_CHANNEL");
    return;
  }

  if (!servoAttached[index]) {
    servos[index].attach(SERVO_PINS[index]);
    servoAttached[index] = true;
  }

  int angle = SERVO_CENTER_DEG;
  if (value == "OPEN") {
    angle = SERVO_OPEN_DEG;
  } else if (value == "CLOSE" || value == "CLOSED") {
    angle = SERVO_CLOSE_DEG;
  } else if (value == "CENTER") {
    angle = SERVO_CENTER_DEG;
  } else {
    angle = value.toInt();
  }

  angle = constrain(angle, 0, 180);
  servos[index].write(angle);
  Serial.print("OK:SERVO:");
  Serial.print(channel);
  Serial.print(":");
  Serial.println(angle);
}

void handleLight(int pin, const String& value) {
  if (pin < 0 || pin > 53) {
    Serial.println("ERR:LIGHT_PIN");
    return;
  }

  pinMode(pin, OUTPUT);
  if (value == "ON" || value == "SET" || value == "1") {
    digitalWrite(pin, HIGH);
  } else if (value == "OFF" || value == "0") {
    digitalWrite(pin, LOW);
  } else if (value == "TOGGLE") {
    digitalWrite(pin, !digitalRead(pin));
  } else {
    Serial.println("ERR:LIGHT_VALUE");
    return;
  }

  Serial.print("OK:LIGHT:");
  Serial.print(pin);
  Serial.print(":");
  Serial.println(digitalRead(pin) == HIGH ? "ON" : "OFF");
}
