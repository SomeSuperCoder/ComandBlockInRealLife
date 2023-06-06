#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <string.h>
#include <GyverButton.h>
#include <math.h>

using namespace std;

LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x3F for a 16 chars and 2 line display

#define MAIN_PAGE "<!DOCTYPE html> <html lang='en'> <head> <meta charset='UTF-8'> <meta http-equiv='X-UA-Compatible' content='IE=edge'> <meta name='viewport' content='width=device-width, initial-scale=1.0'> <title>Коммандный блок</title> <script defer> function getStatus() { const responseElement = document.getElementsByClassName('status'); console.log(responseElement[0]); fetch('/msg_read_status') .then(response => response.text()) .then(data => responseElement[0].innerHTML = data) .catch(error => console.error(error)); setTimeout(getStatus, 3000); } getStatus(); </script> </head> <body> <main> <fieldset> <legend> <h1>Панель управления коммандным блоком</h1> </legend> <form method='get'> <button type='submit' formaction='/no_color'>Ничего</button> <button type='submit' formaction='/red'>Красный</button> <button type='submit' formaction='/green'>Зелёный</button> <button type='submit' formaction='/blue'>Синий</button> </form> <br> <form action='/send_msg'> <input type='text' name='msg' id='msg'> <button type='submit'>Отправить сообщение</button> <p class='status'></p> </form> </fieldset> </main> </body> </html>"

#define SSID "NN 75"
#define PASSWORD "33101500"

#define MAIN_MENU_OPTION_AMOUNT 2

#define MENU_MILLIS_INTERVAL 50

#define LEFT_BUTTON_PIN D4
#define MIDDLE_BUTTON_PIN D7
#define RIGHT_BUTTON_PIN D6

#define R_PIN 10
#define G_PIN D0
#define B_PIN D5

GButton leftButton(LEFT_BUTTON_PIN);
GButton middleButton(MIDDLE_BUTTON_PIN);
GButton rightButton(RIGHT_BUTTON_PIN);

bool isTryingToConnectToWiFi = false;
bool ipWasDisplayed = false;

unsigned short menuIndex = 0;

unsigned long previousMenuMillis = 0;  // variable to store the previous millis() value

String msg_status = "Ваше сообщение не прочтано";

IPAddress local_IP(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

String mainMenuOptions[MAIN_MENU_OPTION_AMOUNT] = {
  "CN",
  "DS"
  // "CR",
  // "RM"
};

ESP8266WebServer server(80);

void setup() {
  Serial.begin(9600);

  pinMode(R_PIN, OUTPUT);
  pinMode(G_PIN, OUTPUT);
  pinMode(B_PIN, OUTPUT);

  lcd.init();
  lcd.clear();         
  lcd.backlight();      // Make sure backlight is on
  
  lcd.setCursor(2, 0);   //Set cursor to character 2 on line 0
  lcd.print("Welcome!");
  delay(2000);
  lcd.clear();
  // WiFiConnect();

  server.on("/", handleIndex);
  server.on("/msg_read_status", handleMSGStatus);
  server.on("/send_msg", handleSendMessage);
  server.on("/no_color", handleNoColor);
  server.on("/red", handleRed);
  server.on("/green", handleGreen);
  server.on("/blue", handleBlue);
  server.onNotFound(handleNotFound);
  
  server.begin();
}

void loop() {
  server.handleClient();

  leftButton.tick();
  middleButton.tick();
  rightButton.tick();

  if (WiFi.status() == WL_CONNECTED && !ipWasDisplayed) {
    ipWasDisplayed = true;
    lcd.clear();
    
    lcd.print(WiFi.localIP());
    delay(5000);
    lcd.clear();
  }
  if (millis() - previousMenuMillis >= MENU_MILLIS_INTERVAL) {
      displayMenu();
    
    // reset the previousMillis variable
    previousMenuMillis = millis();
  }

  handleMenu();
}

void WiFiConnect() {
  WiFi.begin(SSID, PASSWORD);
  ipWasDisplayed = false;
  isTryingToConnectToWiFi = true;
}

void WiFiDisconnect() {
  WiFi.disconnect();
  lcd.clear();
  lcd.print("Diconnected!");
  delay(2000);
  lcd.clear();
}

// void WiFiCreate() {
//   WiFi.softAPConfig(local_IP, gateway, subnet);
//   WiFi.softAP(SSID, PASSWORD);
// }

// void WiFiRemove() {
//   WiFi.softAPdisconnect(true);
// }

void displayMenu() {
  lcd.clear();
  for (short i = 0; i < MAIN_MENU_OPTION_AMOUNT; i++) {
    if (i == menuIndex) {
      lcd.print(">");
    } else {
      lcd.print(" ");
    }
    lcd.print(mainMenuOptions[i]);  
  }
}

void handleMenu() {
  if (rightButton.isPress()) {
    menuIndex++;
    clampMenuIndex();
    Serial.println("RIGHT!");
  }
  if (leftButton.isPress()) {
    menuIndex--;
    clampMenuIndex();
    Serial.println("LEFT!");
  }
  if (middleButton.isPress()) {
    Serial.println("MIDDLE!");
    switch (menuIndex) {
      case 0:
        WiFiConnect();
        break;
      case 1:
        WiFiDisconnect();
        break;
      // case 2:
      //   WiFiCreate();
      //   break;
      // case 3:
      //   WiFiRemove();
      //   break;
    }
  }
}
void clampMenuIndex() {
  if (menuIndex < 0) {
    menuIndex = 0;
  }
  if (menuIndex >= MAIN_MENU_OPTION_AMOUNT) {
    menuIndex = MAIN_MENU_OPTION_AMOUNT-1;
  }
}

void handleIndex() {
  server.send(200, "text/html", MAIN_PAGE);
}

void handleMSGStatus() {
  server.send(200, "text/plain", msg_status);
}

void handleNotFound() {
  server.send(404, "text/html", "<h1>ОШИБКА 404: Страница не найдена :(</h1>");
}

void handleSendMessage() {
  server.sendHeader("Location", String("/"), true);
  server.send(302, "text/plain", "");

  lcd.clear();
  lcd.setCursor(0, 0);
  if (server.arg("msg") == "") {
    return;
  } else {     //Parameter found

   lcd.print(server.arg("msg"));     //Gets the value of the query parameter

  } 
  middleButton.tick();
  while (!middleButton.isPress()) {
    middleButton.tick();
    delay(50);
  }

  msg_status = "Ура! Ваше сообщение прочитано!";
}

void handleRed() {
  digitalWrite(R_PIN, 1);
  digitalWrite(G_PIN, 0);
  digitalWrite(B_PIN, 0);

  server.sendHeader("Location", String("/"), true);
  server.send(302, "text/plain", "");

}

void handleGreen() {
  digitalWrite(R_PIN, 0);
  digitalWrite(G_PIN, 1);
  digitalWrite(B_PIN, 0);

  server.sendHeader("Location", String("/"), true);
  server.send(302, "text/plain", "");

}

void handleBlue() {
  digitalWrite(R_PIN, 0);
  digitalWrite(G_PIN, 0);
  digitalWrite(B_PIN, 1);

  server.sendHeader("Location", String("/"), true);
  server.send(302, "text/plain", "");
}

void handleNoColor() {
  digitalWrite(R_PIN, 0);
  digitalWrite(G_PIN, 0);
  digitalWrite(B_PIN, 0);

  server.sendHeader("Location", String("/"), true);
  server.send(302, "text/plain", "");  
}
