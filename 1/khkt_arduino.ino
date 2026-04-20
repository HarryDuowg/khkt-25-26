int R[2] = {2,5};
int Y[2] = {3,6};
int G[2] = {4,7};
//      data - latch - clock 
int LED1[3] = {8,9,10};
int LED2[3] = {11,12,13};
int L[4] = {0, 0, 0, 0};

const int tat = LOW;
const int bat = HIGH;

bool yolo_ok = false;

void xanh_1_2(int mode){ 
  digitalWrite(G[0], mode);
}
void xanh_3_4(int mode){
  digitalWrite(G[1], mode);
}

void do_1_2(int mode){
  digitalWrite(R[0], mode);
}
void do_3_4(int mode){ 
  digitalWrite(R[1], mode);
}

void vang_1_2(int mode){ 
  digitalWrite(Y[0], mode);
}
void vang_3_4(int mode){ 
  digitalWrite(Y[1], mode);
}

byte ma_mau[] = {
  0b11000000, // 0
  0b11111001, // 1
  0b10100100, // 2
  0b10110000, // 3
  0b10011001, // 4
  0b10010010, // 5
  0b10000010, // 6
  0b11111000, // 7
  0b10000000, // 8
  0b10010000  // 9
};

byte mau_chon[] = {0b00001000, 0b00000100, 0b00000010, 0b00000001};

int den1[4] = {0,0,0,0};
int den2[4] = {0,0,0,0};

void stop() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(R[i], bat);
    digitalWrite(Y[i], tat);
    digitalWrite(G[i], tat);
  }
}

void restart_l() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(LED1[1], tat);
    shiftOut(LED1[0], LED1[2], MSBFIRST, ma_mau[den1[i]]);
    shiftOut(LED1[0], LED1[2], MSBFIRST, mau_chon[i]);
    digitalWrite(LED1[1], bat);

    digitalWrite(LED2[1], tat);
    shiftOut(LED2[0], LED2[2], MSBFIRST, ma_mau[den2[i]]);
    shiftOut(LED2[0], LED2[2], MSBFIRST, mau_chon[i]);
    digitalWrite(LED2[1], bat);

    delayMicroseconds(1500);
  }
}

void set_time(int s) {
  den1[2] = s / 10;
  den1[3] = s % 10;
  den2[2] = s / 10;
  den2[3] = s % 10;
}
// 1 - xanh12 | 2 - vang12 | 3 - xanh34 | 4 - vang34
void t1s(){
  unsigned long t = millis();
  while(millis() - t < 1000){
    restart_l();
  }
}

void read_now(){
  if (Serial.available()) {
    String s = Serial.readStringUntil('\n');
    if (s.startsWith("L1")) {
      read(s);   // cập nhật L[] và P
    }
  }
}


void dem(int s, int mode){
  for (int i = s; i >= 0; i--) {
    set_time(i);

    if (mode == 1) {
      do_1_2(tat); xanh_1_2(bat);
    }
    if (mode == 2) {
      xanh_1_2(tat); vang_1_2(bat);
    }
    if (mode == 3) {
      do_3_4(tat); xanh_3_4(bat);
    }
    if (mode == 4) {
      xanh_3_4(tat); vang_3_4(bat);
    }
    t1s();
  }
  vang_1_2(tat);
  vang_3_4(tat);
}


void setup() {
  Serial.begin(9600);
  Serial.setTimeout(50);
  for (int i = 0; i < 2; i++) {
    pinMode(R[i], OUTPUT);
    pinMode(Y[i], OUTPUT);
    pinMode(G[i], OUTPUT);
  }
  pinMode(LED1[0], OUTPUT);
  pinMode(LED1[1], OUTPUT);
  pinMode(LED1[2], OUTPUT);

  pinMode(LED2[0], OUTPUT);
  pinMode(LED2[1], OUTPUT);
  pinMode(LED2[2], OUTPUT);
  stop();
}
int P=0;

void read(String s){
  sscanf(s.c_str(),
    "L1:%d,L2:%d,L3:%d,L4:%d,P:%d",
    &L[0], &L[1], &L[2], &L[3], &P
  );
}

int xe=2;

bool SAFE(){
  for (int i = 0; i < 4; i++) {
    if (L[i] > 0) return false;
  }
  return true;
}

bool UUTIEN(){
  if(P == 1 || P == 2 || P == 3 || P == 4 ) return true;
  else return false;
}


int dong_xe() { 
  int ans = 0; 
  for(int i = 1; i < 4; i++){
    if(L[i] > L[ans]) ans = i; 
  } 
  return ans; 
}

void d_khien() {
  // 8
  if(SAFE()){
    // time
    int g=10;
    int y=3;

    // 1 - 2  
    //do_1_2(tat);
    dem(g,1);
    dem(y,2);
    do_1_2(bat);

    // 3 - 4
    dem(g,3);
    dem(y,4);
    do_3_4(bat);
  }
  else{
    int l12=0, l34=0;
    l12=max(L[0],L[1]);
    l34=max(L[2],L[3]);

    int gu=20;
    int vu=5;
    int g=10;
    int v=2;
    int bonus = 5;
    int xanh12=g, vang12=v;
    int xanh34=g, vang34=v;
    bool ut12 = false, ut34 = false;
    if(P == 1 || P == 2) ut12 = true;
    if(P == 3 || P == 4) ut34 = true;

    if(ut34) xanh12 /= 2;
    if(ut12) xanh34 /= 2;

    if(l12>=xe){
      xanh12=gu;
      vang12=vu;
    }
    if(l12 >= xe && l34 == 0) xanh12+=bonus;
    if(ut12) xanh12+=bonus;
    
    if(l34>=xe){
      xanh34=gu;
      vang34=vu;
    }
    if(l34 >= xe && l12 == 0) xanh34+=bonus;
    if(ut34) xanh34+=bonus;


    dem(xanh12,1);
    dem(vang12,2);
    do_1_2(bat);

    dem(xanh34,3);
    dem(vang34,4);
    do_3_4(bat);


  }
}


void send_dt() {
  Serial.print("DATA,");
  Serial.print(L[0]); Serial.print(",");
  Serial.print(L[1]); Serial.print(",");
  Serial.print(L[2]); Serial.print(",");
  Serial.print(L[3]);
  Serial.println();
}


bool run   = false;
bool RE = true;

void loop(){

  if (RE && !run) {
    Serial.println("READY");
    RE = false;
  }
  if (!run && Serial.available()) {
    String s = Serial.readStringUntil('\n');
    if (s.startsWith("L1")) {
      read(s);
     run = true;
    }
  }
  if(run) {
    send_dt();
    d_khien();
    run = false;
    RE = true;
    Serial.println("READY");
  }
}

