/*
 * leftLed 為左邊燈的狀態(1 = 亮, 0 = 不亮)
 * rightLed 為右邊燈的狀態(1 = 亮, 0 = 不亮)
 * sameTimePress 紀錄兩顆按鈕是否同時按下
 */ 
int leftLed = 0, rightLed = 0, sameTimePress = 0;
void setup() {
  pinMode(12, OUTPUT); // 12腳用為輸出左邊燈亮暗訊號腳位
  pinMode(13, OUTPUT); // 12腳用為輸出右邊燈亮暗訊號腳位
  pinMode(6, INPUT); // 接收左邊按鈕訊號
  pinMode(7, INPUT); // 接受右邊按鈕訊號
  randomSeed(analogRead(0)); // 隨機亂數種子
}

void loop() {
  /* 
   * l 為左邊那顆按鈕的狀態(pressed or not)
   * r 為右邊那顆按鈕的狀態(pressed or not)
   */
  int l = digitalRead(6), r = digitalRead(7);
  
  // (左邊按鈕按下且右邊不按) 或 (兩邊按鈕一起按)
  if ((l && !r) || l && r) {
  	leftLed = 1;
    rightLed = 0;  
    if (l && r) sameTimePress = 1; // 同時按下兩顆按鈕
  // 左邊按鈕不按且右邊按鈕按下
  } else if (!l && r) {
  	leftLed = 0;
    rightLed = 1;
  } else {
    if (!sameTimePress) return; 
    // 同時按下且放開後隨機亮一顆
    leftLed = random(2);
    rightLed = !leftLed;
    sameTimePress = 0;
  }
  
  // 輸入訊號給12 13腳位
  digitalWrite(12, leftLed);
  digitalWrite(13, rightLed);
}
