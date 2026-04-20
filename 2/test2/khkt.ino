int R[2] = {2, 5};
int Y[2] = {3, 6};
int G[2] = {4, 7};
int LED1[3] = {8, 9, 10};
int LED2[3] = {11, 12, 13};
int L[4] = {0, 0, 0, 0};
int  P = 0;

unsigned long lastTick = 0;
int time1 = 5, time2 = 7;  // 1 do - 2 xanh
int state = 0; 
bool special = false; 

const int N_XE = 3; 

byte ma_mau[] = {0b11000000, 0b11111001, 0b10100100, 0b10110000, 0b10011001, 0b10010010, 0b10000010, 0b11111000, 0b10000000, 0b10010000};
byte mau_chon[] = {0b00001000, 0b00000100, 0b00000010, 0b00000001};

void setLeds(int g1, int y1, int r1, int g2, int y2, int r2) {
  digitalWrite(G[0], g1); digitalWrite(Y[0], y1); digitalWrite(R[0], r1);
  digitalWrite(G[1], g2); digitalWrite(Y[1], y2); digitalWrite(R[1], r2);
}

void update_time() {
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
int x_normal=23;
int x_busy=30;
int v_normal=3;
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

void xu_li_dac_biet(){
  // Xe ưu tiên hướng 3-4, hướng 1-2 đang xanh
  if ((P == 3 || P == 4) && state == 0){
    int tmp=min(10,time1-3);
    time1-=tmp;
    time2-=tmp;
    //time1 = 2; time2 = 5;
    special = true;
  }
   // Xe ưu tiên hướng 1-2, hướng 3-4 đang xanh
  else if ((P == 1 || P == 2) && state == 2) {
    int tmp=min(10,time2-3);
    time2-=tmp;
    time1-=tmp;
    //time2 = 2; time1 = 5;
    special = true;
  }
}

void normal() {
  int l12 = L[0] + L[1];
  int l34 = L[2] + L[3];

  switch(state) {
    case 0: // XANH L1-2
      //setLeds(HIGH, LOW, LOW, LOW, LOW, HIGH); // G1 bat, R2 bat
      if (time1 == 0) { 
        state = 1; 
        time1 = 3; // vang
        special = false; 
      }
      break;
    case 1: // VÀNG L1-2
      //setLeds(LOW, HIGH, LOW, LOW, LOW, HIGH); // Y1 bat, R2 bat
      if (time1 == 0) { 
        state = 2; 
        // xanh l3-l4
        if (l12 < N_XE && l34 < N_XE){  // Safe Mode 
          time2 = x_normal; 
          time1 = x_normal+v_normal; 
        }
        else if (l34 >= N_XE){  // kẹt xe
          time2 = x_busy; 
          time1 = x_busy+v_normal;
        }        
        else{ 
          time1 = x_normal; 
          time2 = x_normal+v_normal; 
        }         
      }
      break;
    case 2: // XANH L3-4
      //setLeds(LOW, LOW, HIGH, HIGH, LOW, LOW); // R1 bat, G2 bat
      if (time2 == 0) { 
        state = 3; 
        time2 = 3; 
        special = false; 
      }
      break;
      
    case 3: // VÀNG L3-4
      //etLeds(LOW, LOW, HIGH, LOW, HIGH, LOW); // R1 bat, Y2 bat
      if (time2 == 0) {
        state = 0; 
        if(l12 < N_XE && l34 < N_XE){ // safe mode
          time1 = x_normal; 
          time2 = x_normal+v_normal; 
        } 
        else if(l12 >= N_XE){ // kẹt xe
          time1 = x_normal; 
          time2 = x_busy+v_normal; 
        }      
        else{ 
          time1 = x_normal; 
          time2 = x_normal+v_normal; 
        }      
        Serial.println("READY");
      }
      break;
  }
}

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 2; i++) {
    pinMode(R[i], OUTPUT); 
    pinMode(Y[i], OUTPUT); 
    pinMode(G[i], OUTPUT);
  }
  for (int i = 0; i < 3; i++) {
    pinMode(LED1[i], OUTPUT); 
    pinMode(LED2[i], OUTPUT);
  }
  Serial.println("READY"); 
}

void loop() {
  if (Serial.available()) {
    String s = Serial.readStringUntil('\n');
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

  update_time();

  set_by_state();

  if (millis() - lastTick >= 1000) {
   lastTick = millis();
   if (time1 > 0) time1--;
   if (time2 > 0) time2--;
  normal();
}

}



