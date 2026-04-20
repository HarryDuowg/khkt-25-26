int R[2] = {2, 5}, Y[2] = {3, 6}, G[2] = {4, 7};
int LED1[3] = {8, 9, 10}, LED2[3] = {11, 12, 13};
int L[4] = {0, 0, 0, 0}, P = 0;

unsigned long lastTick = 0;
int time1 = 10, time2 = 12; 
int state = 0; 
bool specialProcessed = false; 

const int N_XE = 3; 

byte ma_mau[] = {0b11000000, 0b11111001, 0b10100100, 0b10110000, 0b10011001, 0b10010010, 0b10000010, 0b11111000, 0b10000000, 0b10010000};
byte mau_chon[] = {0b00001000, 0b00000100, 0b00000010, 0b00000001};

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 2; i++) {
    pinMode(R[i], OUTPUT); pinMode(Y[i], OUTPUT); pinMode(G[i], OUTPUT);
  }
  for (int i = 0; i < 3; i++) {
    pinMode(LED1[i], OUTPUT); pinMode(LED2[i], OUTPUT);
  }
  Serial.println("READY"); 
}

void loop() {
  if (Serial.available()) {
    String s = Serial.readStringUntil('\n');
    if (s.startsWith("L1")) {
      sscanf(s.c_str(), "L1:%d,L2:%d,L3:%d,L4:%d,P:%d", &L[0], &L[1], &L[2], &L[3], &P);
      
      // A. XỬ LÝ ƯU TIÊN (TRỪ 10S)
      if (!specialProcessed) {
        handlePriorityLogic();
      }

      // B. XỬ LÝ GIA HẠN LÀN TRỐNG (+7S)
      if (state == 0 && !specialProcessed && (L[0] + L[1] > 0) && (L[2] + L[3] == 0)) {
         time1 += 7; time2 += 7;
         specialProcessed = true; 
         Serial.println("BONUS: L1-2 +7s");
      }
      if (state == 2 && !specialProcessed && (L[2] + L[3] > 0) && (L[0] + L[1] == 0)) {
         time2 += 7; time1 += 7;
         specialProcessed = true;
         Serial.println("BONUS: L3-4 +7s");
      }
    }
  }

  updateDisplay();

  if (millis() - lastTick >= 1000) {
    lastTick = millis();
    if (time1 > 0) time1--;
    if (time2 > 0) time2--;
    runStateMachine();
  }
}

void handlePriorityLogic() {
  // Ưu tiên L3-4 (P=3,4) khi L1-2 đang xanh (state 0)
  if ((P == 3 || P == 4) && state == 0) {
    if (time1 > 10) {
      time1 -= 10;
      time2 -= 10;
    } else {
      time1 = 0; // Chuyển sang vàng ngay
      time2 = (time2 > time1) ? (time2 - time1) : 3; // Đồng bộ đèn đỏ hướng kia
    }
    specialProcessed = true;
    Serial.println("PRIORITY: Minus 10s L1-2");
  }
  // Ưu tiên L1-2 (P=1,2) khi L3-4 đang xanh (state 2)
  else if ((P == 1 || P == 2) && state == 2) {
    if (time2 > 10) {
      time2 -= 10;
      time1 -= 10;
    } else {
      time2 = 0; // Chuyển sang vàng ngay
      time1 = (time1 > time2) ? (time1 - time2) : 3;
    }
    specialProcessed = true;
    Serial.println("PRIORITY: Minus 10s L3-4");
  }
}

void runStateMachine() {
  int l12 = L[0] + L[1];
  int l34 = L[2] + L[3];
  bool isSafeMode = (l12 < N_XE && l34 < N_XE);

  switch (state) {
    case 0: // XANH L1-2
      setLeds(HIGH, LOW, LOW, LOW, LOW, HIGH);
      if (time1 == 0) { 
        state = 1; 
        time1 = isSafeMode ? 2 : 3; // Vàng safe 2s, đặc biệt 3s
        specialProcessed = false; 
      }
      break;
      
    case 1: // VÀNG L1-2
      setLeds(LOW, HIGH, LOW, LOW, LOW, HIGH);
      if (time1 == 0) { 
        state = 2; 
        if (isSafeMode) { time2 = 10; time1 = 12; } // Safe Mode
        else { time2 = 20; time1 = 23; }            // Tất cả trường hợp đặc biệt
      }
      break;
      
    case 2: // XANH L3-4
      setLeds(LOW, LOW, HIGH, HIGH, LOW, LOW);
      if (time2 == 0) { 
        state = 3; 
        time2 = isSafeMode ? 2 : 3; 
        specialProcessed = false; 
      }
      break;
      
    case 3: // VÀNG L3-4
      setLeds(LOW, LOW, HIGH, LOW, HIGH, LOW);
      if (time2 == 0) {
        state = 0; 
        if (isSafeMode) { time1 = 10; time2 = 12; } // Safe Mode
        else { time1 = 20; time2 = 23; }            // Tất cả trường hợp đặc biệt
        Serial.println("READY"); 
      }
      break;
  }
}

void setLeds(int g1, int y1, int r1, int g2, int y2, int r2) {
  digitalWrite(G[0], g1); digitalWrite(Y[0], y1); digitalWrite(R[0], r1);
  digitalWrite(G[1], g2); digitalWrite(Y[1], y2); digitalWrite(R[1], r2);
}

void updateDisplay() {
  int d1[4] = {0, 0, time1 / 10, time1 % 10};
  int d2[4] = {0, 0, time2 / 10, time2 % 10};
  for (int i = 0; i < 4; i++) {
    hienThi(LED1, d1[i], i);
    hienThi(LED2, d2[i], i);
    delayMicroseconds(600);
  }
}

void hienThi(int led[], int so, int vtri) {
  digitalWrite(led[1], LOW);
  shiftOut(led[0], led[2], MSBFIRST, ma_mau[so]);
  shiftOut(led[0], led[2], MSBFIRST, mau_chon[vtri]);
  digitalWrite(led[1], HIGH);
}