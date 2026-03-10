// BLYNK CONFIGURATION - ĐIỀN THÔNG TIN CỦA BẠN VÀO ĐÂY

#define BLYNK_TEMPLATE_ID "TMPL6KwwT1bHT"
#define BLYNK_TEMPLATE_NAME "LED ESP32"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>


char ssid[] = "Huynh Gia Phat 01"; // Tên Wifi nhà bạn
char pass[] = "22446688";    // Mật khẩu Wifi

// (LEDS) ESP32
int R[2] = {25, 14}; 
int Y[2] = {26, 12}; 
int G[2] = {27, 13}; 

// CẤU HÌNH 2 BẢNG LED MA TRẬN MAX7219
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4 

// MODULE 1 (Dành cho Làn 1-2)
#define DATA_PIN1  19
#define CS_PIN1    18 
#define CLK_PIN1   5  

// MODULE 2 (Dành cho Làn 3-4)
#define DATA_PIN2  17 
#define CS_PIN2    16 
#define CLK_PIN2   4  

MD_Parola display1 = MD_Parola(HARDWARE_TYPE, DATA_PIN1, CLK_PIN1, CS_PIN1, MAX_DEVICES);
MD_Parola display2 = MD_Parola(HARDWARE_TYPE, DATA_PIN2, CLK_PIN2, CS_PIN2, MAX_DEVICES);

int L[4] = {0, 0, 0, 0};
int P = 0;

unsigned long lastTick = 0;

int x_normal = 23;
int x_busy = 30;
int v_normal = 3;

// 1 do - 2 xanh 
// Lúc đầu: time1 là đỏ (do 1 chờ 2 xanh) -> time1 = 23, time2 = 23 + 3 = 26
int time1 = x_normal, time2 = x_normal + v_normal;  
int state = 0; 
bool special = false; 

const int N_XE = 3; 

BlynkTimer timer;

// Điều khiển Output các LED đỏ vàng xanh
void setLeds(int g1, int y1, int r1, int g2, int y2, int r2) {
  digitalWrite(G[0], g1); digitalWrite(Y[0], y1); digitalWrite(R[0], r1);
  digitalWrite(G[1], g2); digitalWrite(Y[1], y2); digitalWrite(R[1], r2);
}

void set_by_state() {
  switch (state) {
    case 0: // xanh L1-2
      setLeds(HIGH, LOW, LOW,  LOW, LOW, HIGH);
      break;
    case 1: // vàng L1-2
      setLeds(LOW, HIGH, LOW,  LOW, LOW, HIGH);
      break;
    case 2: // xanh L3-4
      setLeds(LOW, LOW, HIGH,  HIGH, LOW, LOW);
      break;
    case 3: // vàng L3-4
      setLeds(LOW, LOW, HIGH,  LOW, HIGH, LOW);
      break;
  }
}

// Thay thế update_time cũ (dùng shiftOut 3641bs) thành update_time_display (MAX7219)
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

void xu_li_dac_biet(){
  // Xe ưu tiên hướng 3-4, hướng 1-2 đang xanh
  if ((P == 3 || P == 4) && state == 0){
    int tmp = min(10, time1 - 3);
    time1 -= tmp;
    time2 -= tmp;
    special = true;
  }
   // Xe ưu tiên hướng 1-2, hướng 3-4 đang xanh
  else if ((P == 1 || P == 2) && state == 2) {
    int tmp = min(10, time2 - 3);
    time2 -= tmp;
    time1 -= tmp;
    special = true;
  }
}

void normal() {
  int l12 = L[0] + L[1];
  int l34 = L[2] + L[3];

  switch(state) {
    case 0: // XANH L1-2
      if (time1 == 0) { 
        state = 1; 
        time1 = 3; // vang
        special = false; 
      }
      break;
    case 1: // VÀNG L1-2
      if (time1 == 0) { 
        state = 2; 
        // xanh l3-l4
        if (l12 < N_XE && l34 < N_XE){  // Safe Mode 
          time2 = x_normal; 
          time1 = x_normal + v_normal; 
        }
        else if (l34 >= N_XE){  // kẹt xe
          time2 = x_busy; 
          time1 = x_busy + v_normal;
        }        
        else{ 
          time1 = x_normal; 
          time2 = x_normal + v_normal; 
        }         
      }
      break;
    case 2: // XANH L3-4
      if (time2 == 0) { 
        state = 3; 
        time2 = 3; 
        special = false; 
      }
      break;
      
    case 3: // VÀNG L3-4
      if (time2 == 0) {
        state = 0; 
        if(l12 < N_XE && l34 < N_XE){ // safe mode
          time1 = x_normal; 
          time2 = x_normal + v_normal; 
        } 
        else if(l12 >= N_XE){ // kẹt xe
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

// Báo cáo lên App Blynk
void sendSensor() {
  Blynk.virtualWrite(V1, time1);
  Blynk.virtualWrite(V2, time2);
  Blynk.virtualWrite(V3, L[0] + L[1]);
  Blynk.virtualWrite(V4, L[2] + L[3]);
  Blynk.virtualWrite(V5, P);
}

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 2; i++) {
    pinMode(R[i], OUTPUT); 
    pinMode(Y[i], OUTPUT); 
    pinMode(G[i], OUTPUT);
  }

  // Khởi động bảng LED Matrix
  display1.begin();
  display1.setIntensity(5); 
  display1.displayClear();
  display1.print("RDY1");
  
  display2.begin();
  display2.setIntensity(5); 
  display2.displayClear();
  display2.print("RDY2");

  // Thiết lập timer báo cáo Blynk mỗi 1 giây. Nếu dùng blynk thì bỏ comment dòng Blyn.begin()
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  timer.setInterval(1000L, sendSensor);

  Serial.println("ESP32 READY"); 
}

void loop() {
  Blynk.run();
  timer.run();

  if (Serial.available()) {
    String s = Serial.readStringUntil('\n');
    s.trim();
    if (s.startsWith("L1")) {
      sscanf(s.c_str(), "L1:%d,L2:%d,L3:%d,L4:%d,P:%d", &L[0], &L[1], &L[2], &L[3], &P);

      if (!special) {
        xu_li_dac_biet();
      }

      // L1-2 xanh - có xe, L3-4 trống -> bonus 7s cho L1-2
      if (state == 0 && !special && (L[0] + L[1] > 0) && (L[2] + L[3] == 0)) {
         time1 += 7; time2 += 7;
         special = true; 
      }
      
      // L3-4 xanh - có xe, L1-2 trống -> bonus 7s cho L3-4
      if (state == 2 && !special && (L[2] + L[3] > 0) && (L[0] + L[1] == 0)) {
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