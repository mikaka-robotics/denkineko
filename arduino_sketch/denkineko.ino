#include <Adafruit_NeoPixel.h> //ライブラリの読み込み
#include <Adafruit_TiCoServo.h>
#include <MsTimer2.h>

#define LEDPIN 2
#define NUMLED 1
#define DELAY_TIME 100 //待ち時間1の設定

const int SENSOR_1_OUT = 8;
const int SENSOR_1_IN = 7;
const int threshold = 6;

int MOTER_PWN1 = 5;
int MOTER_PWN2 = 10;

float target_angle_neck = 90;
float current_angle_neck = 90;

float target_angle_body = 90;
float current_angle_body = 90;

Adafruit_TiCoServo myservo_neck; 
Adafruit_TiCoServo myservo_body;  

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMLED, LEDPIN, NEO_RGB + NEO_KHZ800);
uint16_t led_max_value = 70;  // 輝度の設定をする(暗い←0 ～ 255→明い)
uint32_t base_color = strip.Color(0, 0, led_max_value); //LEDの色

uint16_t steps = 0;
String robot_status = "sleep";
String led_status = "";
String action = "";

unsigned long now;
unsigned long last_touched_time = 0; //チャタリング防止用
unsigned long last_interaction_time = 0; //自動sleep用
unsigned long action_start_time = 0; //動作生成用
int body_angle = 90; //体の向き (センター位置 記憶用)

void setup() {
  pinMode(SENSOR_1_OUT, OUTPUT);
  pinMode(SENSOR_1_IN, INPUT);
  Serial.begin(9600);
  delay(500);
  myservo_neck.attach(MOTER_PWN1, 500, 2400);
  myservo_neck.write(70); 
  myservo_body.attach(MOTER_PWN2, 500, 2400);
  myservo_body.write(90); 

  //LEDの設定
  strip.begin();   //インスタンスの使用を開始、この時全てのLEDの状態を「0」とする。
  strip.setBrightness(led_max_value);
  strip.show(); //Arduinoから全てのLEDへオフ信号「0」を転送し初期化する。

  //タイマー
  MsTimer2::set(10, timerISR);
  MsTimer2::start();

  last_interaction_time = millis();
  
  delay(500);
  Serial.println("ready");
}

uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color((WheelPos * 3)*led_max_value/255, (255 - WheelPos * 3)*led_max_value/255, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color((255 - WheelPos * 3)*led_max_value/255, 0, (WheelPos * 3)*led_max_value/255);
  } else {
   WheelPos -= 170;
   return strip.Color(0, (WheelPos * 3)*led_max_value/255, (255 - WheelPos * 3)*led_max_value/255);
  }
}

// LEDを連続的に変化させる関数
void controlLED() {
  steps=steps+1; //0~255をインクリメントするだけの変数
  if(steps>255){steps=0;}
  
  if(robot_status == "sleep"){
    int k=steps;
    if(k>125){k=256-k;} //ゆっくり明滅
      strip.setPixelColor(0, strip.Color(k*led_max_value/256,k*led_max_value/256,k*led_max_value/256));
      strip.show();
  }else if(led_status == "rainbow"){
      strip.setPixelColor(0, Wheel((steps) & 255));
      strip.show();
  }else if(led_status == "lipsync"){
    if(steps%5==0){
        int randNum = random(30); //ランダムに明滅させる
        if (randNum < 15) {
            strip.setPixelColor(0, base_color);
        } else {
          strip.setPixelColor(0,strip.Color(0,0,0));
        }
        strip.show();
    }
  }else if(led_status == "touch"){
      strip.setPixelColor(0, strip.Color(led_max_value, 0, 0)); //RED
      strip.show();
  }else if(led_status == "off"){
      strip.setPixelColor(0, strip.Color(0, 0, 0)); 
      strip.show();
  }else if(strip.getPixelColor(0) != base_color){
      strip.setPixelColor(0, base_color);
      strip.show();
  }
}

void timerISR() {
  now = millis();
  controlLED();

  if(robot_status == "sleep"){
    target_angle_neck = 74+6*sin(now/600.); //sin関数で首を上下に動かす
  }

  if(action == "nod" & (now-action_start_time)<2*PI*200){
    target_angle_neck = 70 - 8*sin((now-action_start_time)/200.+PI/4.); //うなずく sin関数で首を上下に動かす
  }
  if(action == "nodnod" & (now-action_start_time)<2*PI*400){
    target_angle_neck = 70 - 8*sin((now-action_start_time)/200.+PI/4.); //2回 うなずく
  }
  if(action == "shake_body" & (now-action_start_time)<2*PI*200){
    target_angle_body = body_angle - 12*sin((now-action_start_time)/200); //sin関数で体を左右に振らす
  }
  
  
  int diff = (target_angle_neck - current_angle_neck); //操作量
  diff = constrain(diff, -1, 1); //一度の操作量を制限する
  current_angle_neck = current_angle_neck + diff; //操作量を加える
  current_angle_neck = constrain(current_angle_neck, 65, 90);
  myservo_neck.write((int)current_angle_neck); 

  diff = (target_angle_body-current_angle_body);
  diff = constrain(diff, -1, 1);
  current_angle_body = current_angle_body + diff;
  current_angle_body = constrain(current_angle_body, 0, 180);
  myservo_body.write((int)current_angle_body); 

  //静電容量によるタッチ判定
  int counter=0;
  digitalWrite(SENSOR_1_OUT, HIGH);
  while (digitalRead(SENSOR_1_IN)!=HIGH) counter++;
  digitalWrite(SENSOR_1_OUT, LOW);  

  if (counter > threshold) { //タッチされた
    led_status = "touch";
    if (now - last_touched_time > 300){ //チャタリング防止
        Serial.println("touch");
        robot_status = "ready";
        target_angle_neck=65;
    }
    strip.show();
    last_touched_time = now;
    last_interaction_time = now;
  }

  if(now - last_touched_time > 300 & led_status=="touch"){
    led_status = "";  
  }
  
  if(((now - last_interaction_time)/1000 > 2*60) & (now - last_touched_time > 10*1000)){ //最後にインタラクションしてから2分たったらsleepモード
    robot_status = "sleep";
    if((now - last_interaction_time)/1000 > 60*60){ //さらに、最後にインタラクションしてから60分たっていたら停止させる
      robot_status = "ready";
      led_status = "";
      target_angle_neck = 75;
    }
  }
}

void loop() {
  if (Serial.available() > 0) {
    String received_data = Serial.readStringUntil('\n'); // 改行文字が来るまでの文字列を読み込む
    parseCommands(received_data);
    last_interaction_time = millis();
  }
  
}

//カンマ区切りになっているデータをパースする
void parseCommands(String input) {
  int startIndex = 0;
  while (startIndex < input.length()) {
    int endIndex = input.indexOf(',', startIndex);
    if (endIndex == -1) {
      endIndex = input.length();
    }

    String command = input.substring(startIndex, endIndex);
    command.trim();
    processCommand(command);
    startIndex = endIndex + 1;
  }
}

int getSeparatorIndex(String command){
  int separatorIndex = -1;
  for (int i = 0; i < command.length(); i++) {
    if (command.charAt(i) >= '0' && command.charAt(i) <= '9') {
      separatorIndex = i;
      break;
    }
  }

  if (separatorIndex == -1) {
    return command.length();
  }
  return separatorIndex;
}

bool isNumeric(String str) {
  for (int i = 0; i < str.length(); i++) {
    if (!isDigit(str.charAt(i))) {
      return false;
    }
  }
  return true;
}

void processCommand(String command) {
  int separatorIndex = getSeparatorIndex(command);
  String key = command.substring(0, separatorIndex);
  String valueString = command.substring(separatorIndex);
  int value = 0;
  if (isNumeric(valueString)) {
    value = valueString.toInt();
  }
  
  if(key=="rainbow"){
    robot_status = "ready";
    led_status = "rainbow";
  }else if(key=="sleep"){
    robot_status = "sleep";
    led_status = "";
  }else if(key=="lipsync"){
    robot_status = "ready";
    led_status = "lipsync";
  }else if(key=="ready"){
    led_status = "";
    robot_status = "ready";
  }else if(key=="home"){
    robot_status = "ready";
    target_angle_neck=70;
    target_angle_body=90;
  }else if(key=="blue"){
    base_color = strip.Color(0, 0, led_max_value);
  }else if(key=="yellow"){
    base_color = strip.Color(led_max_value, led_max_value*0.8, 0);
  }else if(key=="red"){
    base_color = strip.Color(led_max_value*0.8, 0, 0);
  }else if(key=="orange"){
    base_color = strip.Color(led_max_value*0.8, led_max_value*0.3, 0);
  }else if(key=="white"){
    base_color = strip.Color(10, 10, 10);
  }else if(key=="bright"){
    led_max_value = value;
    strip.setBrightness(value); //輝度はsetPixelColorにセットした値よりもこっちの方が優先されるよう
    strip.show();
  }else if(key=="neck"){
    robot_status = "ready";
    target_angle_neck=value;
  }else if(key=="body"){
    robot_status = "ready";
    target_angle_body = value;
    body_angle = value;
  }else if(key=="nod"){ //うなずく
    robot_status = "ready";
    action_start_time = millis();
    action = "nod";
  }else if(key=="nodnod"){ //2回うなずく
    robot_status = "ready";
    action_start_time = millis();
    action = "nodnod";
  }else if(key=="shake_body"){ //体を２回左右に揺らす
    robot_status = "ready";
    action_start_time = millis();
    action = "shake_body";
  }else if(key=="status"){
    Serial.println(robot_status);
  }else{
    Serial.println("error");
  }
}
