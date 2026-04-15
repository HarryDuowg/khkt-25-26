// =========================
// BLYNK CONFIGURATION
// =========================
#define BLYNK_TEMPLATE_ID "TMPL6L4IqNXDy"
#define BLYNK_TEMPLATE_NAME "Hello LED"
#define BLYNK_AUTH_TOKEN "oDrb7vKDZNGLMUuGgPT7PHUfyo_McXS3"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

// =========================
// WIFI
// =========================
char ssid[] = "THPTVINHXUAN";
char pass[] = "vx12345678";

// =========================
// LED ĐÈN GIAO THÔNG
// =========================
int R[2] = {25, 14}; 
int Y[2] = {26, 12}; 
int G[2] = {27, 13}; 

// =========================
// LED MATRIX
// =========================
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4 

#define DATA_PIN1  19
#define CS_PIN1    18 
#define CLK_PIN1   5  

#define DATA_PIN2  17 
#define CS_PIN2    16 
#define CLK_PIN2   4  

MD_Parola display1 = MD_Parola(HARDWARE_TYPE, DATA_PIN1, CLK_PIN1, CS_PIN1, MAX_DEVICES);
MD_Parola display2 = MD_Parola(HARDWARE_TYPE, DATA_PIN2, CLK_PIN2, CS_PIN2, MAX_DEVICES);

// =========================
// BIẾN HỆ THỐNG
// =========================
int L[4] = {0, 0, 0, 0};
int P = 0;

unsigned long lastTick = 0;

int x_normal = 23;
int x_busy = 30;
int v_normal = 3;

int time1 = x_normal;
int time2 = x_normal + v_normal;

int state = 0;
bool special = false;

const int N_XE = 3;

BlynkTimer timer;

// =========================
// LED CONTROL
// =========================
void setLeds(int g1, int y1, int r1, int g2, int y2, int r2) {
  digitalWrite(G[0], g1); digitalWrite(Y[0], y1); digitalWrite(R[0], r1);
  digitalWrite(G[1], g2); digitalWrite(Y[1], y2); digitalWrite(R[1], r2);
}

void set_by_state() {
  switch (state) {
    case 0: setLeds(HIGH, LOW, LOW,  LOW, LOW, HIGH); break;
    case 1: setLeds(LOW, HIGH, LOW,  LOW, LOW, HIGH); break;
    case 2: setLeds(LOW, LOW, HIGH,  HIGH, LOW, LOW); break;
    case 3: setLeds(LOW, LOW, HIGH,  LOW, HIGH, LOW); break;
  }
}

// =========================
// HIỂN THỊ LED MATRIX
// =========================
void update_time_display() {
  char buf1[10];
  char buf2[10];

  sprintf(buf1, "%02d", time1);
  sprintf(buf2, "%02d", time2);

  display1.setTextAlignment(PA_CENTER);
  display1.print(buf1);

  display2.setTextAlignment(PA_CENTER);
  display2.print(buf2);
}

// =========================
// XỬ LÝ ƯU TIÊN
// =========================
void xu_li_dac_biet(){
  if ((P == 3 || P == 4) && state == 0){
    int tmp = min(10, time1 - 3);
    time1 -= tmp;
    time2 -= tmp;
    special = true;
  }
  else if ((P == 1 || P == 2) && state == 2){
    int tmp = min(10, time2 - 3);
    time2 -= tmp;
    time1 -= tmp;
    special = true;
  }
}

// =========================
// LOGIC ĐÈN
// =========================
void normal() {
  int l12 = L[0] + L[1];
  int l34 = L[2] + L[3];

  switch(state) {
    case 0:
      if (time1 == 0) {
        state = 1;
        time1 = 3;
        special = false;
      }
      break;

    case 1:
      if (time1 == 0) {
        state = 2;

        if (l12 < N_XE && l34 < N_XE){
          time2 = x_normal;
          time1 = x_normal + v_normal;
        }
        else if (l34 >= N_XE){
          time2 = x_busy;
          time1 = x_busy + v_normal;
        }
        else{
          time1 = x_normal;
          time2 = x_normal + v_normal;
        }
      }
      break;

    case 2:
      if (time2 == 0) {
        state = 3;
        time2 = 3;
        special = false;
      }
      break;

    case 3:
      if (time2 == 0) {
        state = 0;

        if (l12 < N_XE && l34 < N_XE){
          time1 = x_normal;
          time2 = x_normal + v_normal;
        }
        else if (l12 >= N_XE){
          time1 = x_normal;
          time2 = x_busy + v_normal;
        }
        else{
          time1 = x_normal;
          time2 = x_normal + v_normal;
        }

        Serial.println("READY");
      }
      break;
  }
}

// =========================
// GỬI BLYNK
// =========================
void sendSensor() {
  Blynk.virtualWrite(V1, time1);
  Blynk.virtualWrite(V2, time2);
  Blynk.virtualWrite(V3, L[0] + L[1]);
  Blynk.virtualWrite(V4, L[2] + L[3]);
  Blynk.virtualWrite(V5, P);
  
}

// =========================
// SETUP
// =========================
void setup() {
  Serial.begin(9600);
  Serial.setTimeout(10);

  for (int i = 0; i < 2; i++) {
    pinMode(R[i], OUTPUT);
    pinMode(Y[i], OUTPUT);
    pinMode(G[i], OUTPUT);
  }

  display1.begin();
  display1.setIntensity(5);
  display1.displayClear();
  display1.print("RDY1");

  display2.begin();
  display2.setIntensity(5);
  display2.displayClear();
  display2.print("RDY2");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  timer.setInterval(2000L, sendSensor);

  Serial.println("ESP32 READY");
}

// =========================
// LOOP
// =========================
void loop() {
  Blynk.run();
  timer.run();

  // Nhận dữ liệu từ Python
if (Serial.available() > 0) {
    String s = Serial.readStringUntil('\n');
    s.trim();
    if (s.length() > 0 && s.startsWith("L1")) {
       sscanf(s.c_str(), "L1:%d,L2:%d,L3:%d,L4:%d,P:%d", &L[0], &L[1], &L[2], &L[3], &P);
      if (!special) xu_li_dac_biet();

      if (state == 0 && !special && (L[0]+L[1] > 0) && (L[2]+L[3] == 0)) {
        time1 += 7; time2 += 7;
        special = true;
      }

      if (state == 2 && !special && (L[2]+L[3] > 0) && (L[0]+L[1] == 0)) {
        time2 += 7; time1 += 7;
        special = true;
      }
    }
  }

  update_time_display();
  set_by_state();

  if (millis() - lastTick >= 1000) {
    lastTick = millis();

    if (time1 > 0) time1--;
    if (time2 > 0) time2--;

    normal();
  }
}
