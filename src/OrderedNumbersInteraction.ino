uint8_t orderedNumbers[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
uint8_t shuffledKeyboardNumbers[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
uint8_t currentIndex = 0;

const uint8_t ledBarGraphPins[10] = {10, 11, 12, 19, 18, 17, 16, 15, 14, 13};
const uint8_t greenLEDPin = 20;
const uint8_t redLEDPin = 21;
const uint8_t buzzerPin = 9;

byte rows[] = {2, 3, 4};
const int rowCount = sizeof(rows) / sizeof(rows[0]);

byte cols[] = {5, 6, 7, 8};
const int colCount = sizeof(cols) / sizeof(cols[0]);

byte keys[colCount][rowCount];
uint8_t numbers[colCount][rowCount] = {
  {11, 0, 11},
  {9, 8, 7},
  {6, 5, 4},
  {3, 2, 1}
};

bool INTERACTION_SOLVED, INTERACTION_RUNNING;

void setup() {
  Serial.begin(9600);
  for (uint8_t i = 0; i < 10; i++) {
    pinMode(ledBarGraphPins[i], OUTPUT);
  }
  shutdownLedBarGraph();
}

void loop() {
  if (!Serial) {
    Serial.begin(9600);
  }
  if (Serial.available()) {
    processSerialMessage();
  }
  if (INTERACTION_SOLVED == false && INTERACTION_RUNNING == true) {
    gameLoop();
  }
}

void gameLoop() {
  uint8_t pressedKey = readMatrixAndReturnPressedKey();
  if (pressedKey != 11) {
    selectNumber(pressedKey);
  }
  delay(250);
}

uint8_t readMatrixAndReturnPressedKey() {
  uint8_t pressedKey = 11;
  while (pressedKey == 11) {
    // iterate the columns
    for (int colIndex = 0; colIndex < colCount; colIndex++) {
      // col: set to output to low
      byte curCol = cols[colIndex];
      pinMode(curCol, OUTPUT);
      digitalWrite(curCol, LOW);

      // row: interate through the rows
      for (int rowIndex = 0; rowIndex < rowCount; rowIndex++) {
        byte rowCol = rows[rowIndex];
        pinMode(rowCol, INPUT_PULLUP);
        keys[colIndex][rowIndex] = digitalRead(rowCol);
        pinMode(rowCol, INPUT);
      }
      // disable the column
      pinMode(curCol, INPUT);
    }

    for (int colIndex = 0; colIndex < colCount; colIndex++) {
      for (int rowIndex = 0; rowIndex < rowCount; rowIndex++) {
        if (keys[colIndex][rowIndex] == LOW) {
          pressedKey = numbers[colIndex][rowIndex];
        }
      }
    }

    if (Serial.available()) {
      processSerialMessage();
      return 11;
    }

  }

  return pressedKey;

}

void selectNumber(uint8_t selectedArrayIndex) {
  uint8_t selectedNumber = shuffledKeyboardNumbers[selectedArrayIndex];
  Serial.print("Selected Array Index: ");
  Serial.println(selectedArrayIndex);
  Serial.print("Selected Number From Shuffled Array: ");
  Serial.println(selectedNumber);
  Serial.print("Number from Ordered Numbers: ");
  Serial.println(orderedNumbers[currentIndex]);
  if (selectedNumber == orderedNumbers[currentIndex]) {
    digitalWrite(ledBarGraphPins[selectedNumber - 1], HIGH);
    if (currentIndex == (sizeof(orderedNumbers) - 1)) {
      checkWinning();
      winningLedBarGraphSequence();
    } else {
      currentIndex++;
    }
  } else {
    digitalWrite(redLEDPin, HIGH);
    digitalWrite(ledBarGraphPins[selectedNumber - 1], HIGH);
    delay(750);
    digitalWrite(ledBarGraphPins[selectedNumber - 1], LOW);
    delay(750);
    digitalWrite(ledBarGraphPins[selectedNumber - 1], HIGH);
    delay(750);
    digitalWrite(ledBarGraphPins[selectedNumber - 1], LOW);
    delay(750);
    digitalWrite(ledBarGraphPins[selectedNumber - 1], HIGH);
    delay(750);
    digitalWrite(ledBarGraphPins[selectedNumber - 1], LOW);
    delay(750);
    digitalWrite(redLEDPin, LOW);
    shutdownLedBarGraph();
    currentIndex = 0;
  }
}

void shutdownLedBarGraph() {
  for (uint8_t i = 0; i < sizeof(ledBarGraphPins); i++) {
    digitalWrite(ledBarGraphPins[i], LOW);
  }
}

void winningLedBarGraphSequence() {
  uint8_t times = 0;
  digitalWrite(greenLEDPin, HIGH);
  while (times < 3) {
    shutdownLedBarGraph();
    for (uint8_t i = 0; i < sizeof(ledBarGraphPins); i++) {
      digitalWrite(ledBarGraphPins[i], HIGH);
      delay(150);
    }
    times++;
  }
}

void shuffleArray() {
  const uint8_t questionCount = sizeof shuffledKeyboardNumbers / sizeof shuffledKeyboardNumbers[0];
  for (uint8_t i = 0; i < questionCount; i++) {
    uint8_t n = random(0, questionCount);  // Integer from 0 to questionCount-1
    uint8_t temp = shuffledKeyboardNumbers[n];
    shuffledKeyboardNumbers[n] =  shuffledKeyboardNumbers[i];
    shuffledKeyboardNumbers[i] = temp;
  }
}

void processSerialMessage() {
  const uint8_t BUFF_SIZE = 64; // make it big enough to hold your longest command
  static char buffer[BUFF_SIZE + 1]; // +1 allows space for the null terminator
  static uint8_t length = 0; // number of characters currently in the buffer

  char c = Serial.read();
  if ((c == '\r') || (c == '\n')) {
    // end-of-line received
    if (length > 0) {
      tokenizeReceivedMessage(buffer);
    }
    length = 0;
  } else {
    if (length < BUFF_SIZE) {
      buffer[length++] = c; // append the received character to the array
      buffer[length] = 0; // append the null terminator
    }
  }
}

void tokenizeReceivedMessage(char *msg) {
  const uint8_t COMMAND_PAIRS = 10;
  char* tokenizedString[COMMAND_PAIRS + 1];
  uint8_t index = 0;

  char* command = strtok(msg, ";");
  while (command != 0) {
    char* separator = strchr(command, ':');
    if (separator != 0) {
      *separator = 0;
      tokenizedString[index++] = command;
      ++separator;
      tokenizedString[index++] = separator;
    }
    command = strtok(0, ";");
  }
  tokenizedString[index] = 0;

  processReceivedMessage(tokenizedString);
}

void processReceivedMessage(char** command) {
  if (strcmp(command[1], "START") == 0) {
    startSequence(command[3]);
  } else if (strcmp(command[1], "PAUSE") == 0) {
    pauseSequence(command[3]);
  } else if (strcmp(command[1], "STOP") == 0) {
    stopSequence(command[3]);
  } else if (strcmp(command[1], "INTERACTION_SOLVED_ACK") == 0) {
    setInteractionSolved();
  } else if (strcmp(command[1], "PING") == 0) {
    ping(command[3]);
  } else if (strcmp(command[1], "BAUD") == 0) {
    setBaudRate(atoi(command[3]), command[5]);
  } else if (strcmp(command[1], "SETUP") == 0) {
    Serial.println("COM:SETUP;INT_NAME:Ordered Numbers Interaction;BAUD:9600");
    Serial.flush();
  }
}

//TODO: Review This Method once Interaction Is Completed
void startSequence(char* TIMESTAMP) {
  randomSeed(millis() * random(1, 1024));
  INTERACTION_SOLVED = false;
  INTERACTION_RUNNING = true;
  shuffleArray();
  Serial.print("COM:START_ACK;MSG:");
  for (uint8_t i = 0; i < sizeof(shuffledKeyboardNumbers); i++) {
    Serial.print(shuffledKeyboardNumbers[i]);
  }
  Serial.print(";ID:");
  Serial.print(TIMESTAMP);
  Serial.print("\r\n");
  Serial.flush();
}

void pauseSequence(char* TIMESTAMP) {
  INTERACTION_RUNNING = !INTERACTION_RUNNING;
  if (INTERACTION_RUNNING) {
    Serial.print("COM:PAUSE_ACK;MSG:Device is now running;ID:");
  } else {
    Serial.print("COM:PAUSE_ACK;MSG:Device is now paused;ID:");
  }
  Serial.print(TIMESTAMP);
  Serial.print("\r\n");
  Serial.flush();
}

void stopSequence(char* TIMESTAMP) {
  INTERACTION_RUNNING = false;
  Serial.print("COM:STOP_ACK;MSG:Device is now stopped;ID:");
  Serial.print(TIMESTAMP);
  Serial.print("\r\n");
  Serial.flush();
}

void setInteractionSolved() {
  INTERACTION_SOLVED = true;
  INTERACTION_RUNNING = false;
}

void ping(char* TIMESTAMP) {
  Serial.print("COM:PING;MSG:PING;ID:");
  Serial.print(TIMESTAMP);
  Serial.print("\r\n");
  Serial.flush();
}

void setBaudRate(int baudRate, char* TIMESTAMP) {
  Serial.flush();
  Serial.begin(baudRate);
  Serial.print("COM:BAUD_ACK;MSG:The Baud Rate was set to ");
  Serial.print(baudRate);
  Serial.print(";ID:");
  Serial.print(TIMESTAMP);
  Serial.print("\r\n");
  Serial.flush();
}

bool checkWinning() {
  Serial.println("COM:INTERACTION_SOLVED;MSG:User Ordered Numbers Correctly;PNT:1250");
  Serial.flush();
}
