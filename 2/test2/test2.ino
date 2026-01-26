int R[2] = {2, 5}, Y[2] = {3, 6}, G[2] = {4, 7};
int LED1[3] = {8, 9, 10}, LED2[3] = {11, 12, 13};
int L[4] = {0, 0, 0, 0}, P = 0;

unsigned long lastTick = 0;
int time1 = 5, time2 = 7; 
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
  // Khởi tạo trạng thái đèn ban đầu ngay lập tức
  updateLights();
  Serial.println("READY"); 
}

void loop() {
  // 1. Nhận dữ liệu từ Serial
  if (Serial.available()) {
    String s = Serial.readStringUntil('\n');
    if (s.startsWith("L1")) {
      sscanf(s.c_str(), "L1:%d,L2:%d,L3:%d,L4:%d,P:%d", &L[0], &L[1], &L[2], &L[3], &P);
      
      if (!specialProcessed) {
        handlePriorityLogic();
      }

      // Gia hạn thời gian nếu làn đối diện trống
      if (state == 0 && !specialProcessed && (L[0] + L[1] > 0) && (L[2] + L[3] == 0)) {
         time1 += 7; time2 += 7;
         specialProcessed = true; 
         Serial.println("BONUS: L1-2 +4s");
      }
      if (state == 2 && !specialProcessed && (L[2] + L[3] > 0) && (L[0] + L[1] == 0)) {
         time2 += 7; time1 += 7;
         specialProcessed = true;
         Serial.println("BONUS: L3-4 +4s");
      }
    }
  }

  // 2. Cập nhật hiển thị LED 7 đoạn (liên tục để không bị nhấp nháy)
  updateDisplay();

  // 3. Xử lý đếm ngược và chuyển trạng thái (Đồng bộ mỗi 1 giây)
  if (millis() - lastTick >= 1000) {
    lastTick = millis();
    
    // Giảm thời gian
    if (time1 > 0) time1--;
    if (time2 > 0) time2--;

    // Kiểm tra chuyển trạng thái ngay khi time chạm 0
    runStateMachine();
    
    // Cập nhật đèn LED giao thông ngay sau khi đổi trạng thái
    updateLights();
  }
}

void handlePriorityLogic() {
  if ((P == 3 || P == 4) && state == 0) { 
    time1 = 2; time2 = 5;
    specialProcessed = true;
    Serial.println("PRIORITY: L3-4");
  }
  else if ((P == 1 || P == 2) && state == 2) { 
    time2 = 2; time1 = 5;
    specialProcessed = true;
    Serial.println("PRIORITY: L1-2");
  }
}

void runStateMachine() {
  int l12 = L[0] + L[1];
  int l34 = L[2] + L[3];

  // Logic chuyển state dựa trên time1 hoặc time2 vừa chạm 0
  if (state == 0 && time1 == 0) {
      state = 1;
      time1 = (l12 < N_XE && l34 < N_XE) ? 2 : 3;
      specialProcessed = false;
  } 
  else if (state == 1 && time1 == 0) {
      state = 2;
      if (l12 < N_XE && l34 < N_XE) { time2 = 5; time1 = 7; }
      else if (l34 >= N_XE) { time2 = 10; time1 = 13; }
      else { time2 = 5; time1 = 8; }
  } 
  else if (state == 2 && time2 == 0) {
      state = 3;
      time2 = (l12 < N_XE && l34 < N_XE) ? 2 : 3;
      specialProcessed = false;
  } 
  else if (state == 3 && time2 == 0) {
      state = 0;
      if (l12 < N_XE && l34 < N_XE) { time1 = 5; time2 = 7; }
      else if (l12 >= N_XE) { time1 = 10; time2 = 13; }
      else { time1 = 5; time2 = 8; }
      Serial.println("READY");
  }
}

// Hàm cập nhật trạng thái đèn LED vật lý
void updateLights() {
  switch (state) {
    case 0: setLeds(HIGH, LOW, LOW, LOW, LOW, HIGH); break; // Xanh 1, Đỏ 2
    case 1: setLeds(LOW, HIGH, LOW, LOW, LOW, HIGH); break; // Vàng 1, Đỏ 2
    case 2: setLeds(LOW, LOW, HIGH, HIGH, LOW, LOW); break; // Đỏ 1, Xanh 2
    case 3: setLeds(LOW, LOW, HIGH, LOW, HIGH, LOW); break; // Đỏ 1, Vàng 2
  }
}

void setLeds(int g1, int y1, int r1, int g2, int y2, int r2) {
  digitalWrite(G[0], g1); digitalWrite(Y[0], y1); digitalWrite(R[0], r1);
  digitalWrite(G[1], g2); digitalWrite(Y[1], y2); digitalWrite(R[1], r2);
}

void updateDisplay() {
  int d1[4] = {0, 0, time1 / 10, time1 % 10};
  int d2[4] = {0, 0, time2 / 10, time2 % 10};
  for (int i = 2; i < 4; i++) { // Chỉ quét 2 số cuối (hàng chục và đơn vị)
    hienThi(LED1, d1[i], i);
    hienThi(LED2, d2[i], i);
    delayMicroseconds(800); 
  }
}

void hienThi(int led[], int so, int vtri) {
  digitalWrite(led[1], LOW);
  shiftOut(led[0], led[2], MSBFIRST, ma_mau[so]);
  shiftOut(led[0], led[2], MSBFIRST, mau_chon[vtri]);
  digitalWrite(led[1], HIGH);
}