#include <SoftwareSerial.h>
/* Include the standard Arduino SPI library */
#include <SPI.h>
/* Include the RFID library */
#include <RFID.h>
#include <Ultrasonic.h>

const int EN1 = 6;
const int EN2 = 3;
const int EN3 = 5;
const int EN4 = 10;
const int IN1 = 7;
const int IN2 = 2;
const int IN3 = 4;
const int IN4 = 12;


const int distThreshold = 5;  // Distance threshold for object detection in centimeters

const int trigPinB = 28;
const int echoPinB = 26;
const int trigPinR = 13;
const int echoPinR = 30;
const int trigPinL = 24;
const int echoPinL = 22;

Ultrasonic ultrasonicf(47);

#define RxD A9
#define TxD A8

SoftwareSerial BTserial(RxD, TxD);

/* Define the DIO used for the SDA (SS) and RST (reset) pins. */
#define SDA_DIO 9
#define RESET_DIO 8
/* Create an instance of the RFID library */
RFID RC522(SDA_DIO, RESET_DIO);

String card = "";
String newCard = "";

String oldDirection = "";
String newDirection = "";


bool newCardDetected;

// Define a structure to store RFID tag values and their corresponding cell coordinates
struct RFID_Cell {
  String tag;
  int x;
  int y;
};

// Define an array of RFID_Cell structures
RFID_Cell cells[] = {
  { "23562AACF3", 0, 0 },  // Cell (0,0)
  { "81241B2698", 0, 1 },  // Cell (0,1)
  { "631F68ACB8", 0, 2 },  // Cell (0,2)
  { "30F54A1D92", 1, 0 },  // Cell (1,0)
  { "83D262AC9F", 1, 1 },  // Cell (1,1)
  { "538767AD1E", 1, 2 },  // Cell (1,2)
  { "E3EB3EAC9A", 2, 0 },  // Cell (2,0)
  { "83B564ADFF", 2, 1 },  // Cell (2,1)
  { "3093A91A10", 2, 2 }   // Cell (2,2)
};

int targetCell;  // User input
int x_tt, y_tt;
int x_diff, y_diff;
int currentX, currentY;
int counter = 0;
int fal7oosa[3][3] = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };

void setup() {
  pinMode(EN1, OUTPUT);
  pinMode(EN2, OUTPUT);
  pinMode(EN3, OUTPUT);
  pinMode(EN4, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);


  pinMode(trigPinB, OUTPUT);  // Sets the trigPin as an Output
  pinMode(echoPinB, INPUT);   // Sets the echoPin as an Input
  pinMode(trigPinR, OUTPUT);  // Sets the trigPin as an Output
  pinMode(echoPinR, INPUT);   // Sets the echoPin as an Input
  pinMode(trigPinL, OUTPUT);  // Sets the trigPin as an Output
  pinMode(echoPinL, INPUT);   // Sets the echoPin as an Input

  digitalWrite(trigPinB, LOW);  // Clear trigPinB
  digitalWrite(trigPinR, LOW);  // Clear trigPinR
  digitalWrite(trigPinL, LOW);  // Clear trigPinL

  BTserial.begin(9600);
  /* Enable the SPI interface */
  SPI.begin();
  /* Initialise the RFID reader */
  RC522.init();

  // Wait until a card is detected
  while (!RC522.isCard())
    ;
  readCard();
  newCard = card;
  currentCell();
  target();  // User input of target cell; calls Delta function
  delay(1000);
}

void loop() {
  // Stop motors and wait for a short time
  stopMotors();
  delay(500);
  oldDirection = newDirection;
  counter++;
  // Check current position
  currentCell();
  delta(x_tt, y_tt);

  // If the target is reached, print a message and stop the loop
  if ((currentX == x_tt) && (currentY == y_tt)) {
    BTserial.println("SOLVED :)");
    counter = 0;
    BTserial.flush();
    target();
    loop();
    // while(1);
  } else {
    // Move towards the target cell
    move1Cell();
  }
}

void Forward() {
  setMotors(0, false, 17, true);
}

void Backward() {
  setMotors(0, false, 17, false);
}

void Right() {
  setMotors(17, false, 0, true);
}

void Left() {
  setMotors(17, true, 0, false);
}

void stopMotors() {
  setMotors(0, false, 0, false);
}

void setMotors(int pwm1, bool reverse1, int pwm2, bool reverse2) {
  analogWrite(EN1, pwm2);  // left false = back
  analogWrite(EN2, pwm1);  // front
  analogWrite(EN3, pwm1);  // back false = right
  analogWrite(EN4, pwm2);  // right

  digitalWrite(IN1, reverse2 ? HIGH : LOW);
  digitalWrite(IN2, reverse1 ? LOW : HIGH);
  digitalWrite(IN3, reverse1 ? HIGH : LOW);
  digitalWrite(IN4, reverse2 ? LOW : HIGH);
}


int readDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  return pulseIn(echoPin, HIGH) * 0.034 / 2;
}

void readCard() {
  card = "";
  // If a card is detected, get its BTserial number
  if (RC522.readCardSerial()) {
    BTserial.println("Card detected:");

    // Concatenate UID bytes to form a string
    for (int i = 0; i < 5; i++) {
      BTserial.print(RC522.serNum[i], HEX);  // Print card details in hexadecimal format
      card.concat(String(RC522.serNum[i], HEX));
    }
    card.toUpperCase();
    BTserial.println();
    if (card == newCard) {
      newCardDetected = false;
      BTserial.println("Cards are equal");
    } else {
      newCard = card;
      BTserial.println("NewCardDetected");
      delay(1000);
      newCardDetected = true;
    }

    BTserial.println();
  }
}

void currentCell() {
  BTserial.println("Current cell is:");

  // Iterate through the cells array to find the current cell
  for (int i = 0; i < sizeof(cells) / sizeof(cells[0]); i++) {
    if (card == cells[i].tag) {
      BTserial.print("Cell (");
      BTserial.print(cells[i].x);
      BTserial.print(",");
      BTserial.print(cells[i].y);
      BTserial.println(")");
      currentX = cells[i].x;
      currentY = cells[i].y;
      BTserial.println();
      return;  // Exit the function once the cell is found
    }
  }
}

// User Input Bluetooth to Target --> Example: 1 --> Cell (x_tt = 0, y_tt = 1)
void target() {
  BTserial.println("Enter target cell");
  while (BTserial.available() == 0)
    ;
  targetCell = BTserial.parseInt();
  BTserial.println(targetCell);

  // Define an array to map user input to target cell coordinates
  int targetCoordinates[][2] = {
    { 0, 0 },  // 0 -> Cell (0,0)
    { 0, 1 },  // 1 -> Cell (0,1)
    { 0, 2 },  // 2 -> Cell (0,2)
    { 1, 0 },  // 3 -> Cell (1,0)
    { 1, 1 },  // 4 -> Cell (1,1)
    { 1, 2 },  // 5 -> Cell (1,2)
    { 2, 0 },  // 6 -> Cell (2,0)
    { 2, 1 },  // 7 -> Cell (2,1)
    { 2, 2 }   // 8 -> Cell (2,2)
  };

  // Check if the user input is within range
  if (targetCell >= 0 && targetCell <= 8) {
    // Set target cell coordinates based on user input
    x_tt = targetCoordinates[targetCell][0];
    y_tt = targetCoordinates[targetCell][1];
    delta(x_tt, y_tt);
    BTserial.println("Target Cell is:");
    BTserial.print("Cell (");
    BTserial.print(x_tt);
    BTserial.print(",");
    BTserial.print(y_tt);
    BTserial.println(")");
    BTserial.println("Difference is:");
    BTserial.print("x_diff: ");
    BTserial.println(x_diff);
    BTserial.print("y_diff: ");
    BTserial.println(y_diff);
  }
}

void delta(int xx, int yy) {
  x_diff = xx - currentX;
  y_diff = yy - currentY;
}

void move1Cell() {
  // Conditions to determine the move direction
  bool canMoveRight = (x_diff > 0) && (fal7oosa[currentX + 1][currentY] == 0) && (currentX < 2);
  bool canMoveLeft = (x_diff < 0) && (fal7oosa[currentX - 1][currentY] == 0) && (currentX > 0);
  bool canMoveForward = (y_diff > 0) && (fal7oosa[currentX][currentY + 1] == 0) && (currentY < 2);
  bool canMoveBackward = (y_diff < 0) && (fal7oosa[currentX][currentY - 1] == 0) && (currentY > 0);

  BTserial.println("Can Move Right");
  BTserial.println(canMoveRight);
  BTserial.println("Can Move Left");
  BTserial.println(canMoveLeft);
  BTserial.println("Can Move Forward");
  BTserial.println(canMoveForward);
  BTserial.println("Can Move Backward");
  BTserial.println(canMoveBackward);

  // Handle horizontal movement priorities
  if (canMoveRight && (x_diff >= y_diff || (!canMoveForward && !canMoveBackward))) {
    moveRight();
  }

  if (canMoveLeft && (-x_diff >= y_diff || !canMoveForward && !canMoveBackward)) {
    moveLeft();
  }

  // Handle vertical movement priorities
  if (canMoveForward && (y_diff >= x_diff || !canMoveRight && !canMoveLeft)) {
    moveForward();
  }

  if (canMoveBackward && (-y_diff >= x_diff || !canMoveRight && !canMoveLeft)) {
    moveBackward();
  }

  // Handling cases where movement is blocked in all directions
  if (!canMoveRight && !canMoveLeft && !canMoveForward && !canMoveBackward) {
    BTserial.println("No move possible!");
    // Resetting the array to zeros
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        fal7oosa[i][j] = 0;
      }
    }

    BTserial.flush();
    target();
    loop();
  }
}

void moveRight() {
  while (readDistance(trigPinR, echoPinR) > distThreshold) {
    Right();
    // BTserial.println("Moving Right");
    if (RC522.isCard()) {
      readCard();
        Left();
        delay(1000);
        BTserial.println("Back to loop");
        loop();
      }
    }
  }

  if (readDistance(trigPinR, echoPinR) <= distThreshold) {
    fal7oosa[currentX + 1][currentY] = 1;
    moveLeft();
  }
}

void moveLeft() {
  while (readDistance(trigPinL, echoPinL) > distThreshold) {
    Left();
    // BTserial.println("Moving Left");
    if (RC522.isCard()) {
      readCard();
      if (newCardDetected) {
        Right();
        delay(1000);
        BTserial.println("Back to loop");
        loop();
      }
    }
  }

  if (readDistance(trigPinL, echoPinL) <= distThreshold) {
    fal7oosa[currentX - 1][currentY] = 1;
    moveRight();
  }
}

void moveForward() {
  while (ultrasonicf.MeasureInCentimeters() > distThreshold) {
    Forward();
    // BTserial.println("Moving Forward");
    if (RC522.isCard()) {
      readCard();
      if (newCardDetected) {
        Backward();
        delay(1000);
        BTserial.println("Back to loop");
        loop();
      }
    }
  }
  if (ultrasonicf.MeasureInCentimeters() <= distThreshold) {
    fal7oosa[currentX][currentY + 1] = 1;
    moveBackward();
  }
}



void moveBackward() {
  while (readDistance(trigPinB, echoPinB) > distThreshold) {
    Backward();
    // BTserial.println("Moving Backward");
    if (RC522.isCard()) {
      readCard();
      if (newCardDetected) {
        Forward();
        delay(1000);
        BTserial.println("Back to loop");
        loop();
      }
    }
  }
  if (readDistance(trigPinB, echoPinB) <= distThreshold) {
    fal7oosa[currentX][currentY - 1] = 1;
    moveForward();
  }
}

void loop() {
  readCard();
  currentCell();
  target();
  while (!solved) {
    solution();
  }
  BTserial.println("Solved");
    if(!item)
    {
      BTserial.println("PickUP POINT");
      digitalWrite(pickUp, HIGH);
      item = true;
    }
    else
    {
      BTserial.println("DROP OFF POINT");
      digitalWrite(drop, HIGH);
      item = false;
    }
}
