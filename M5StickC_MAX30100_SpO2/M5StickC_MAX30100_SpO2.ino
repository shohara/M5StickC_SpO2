#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include <M5StickC.h>
const int LCD_WIDTH = 160;
const int LCD_HEIGHT = 80;
const int THRESH_MUL = 2.5;
const int REPORTING_PERIOD_MS = 1000;
const int MIN_SPO2 = 70;
const int MAX_SPO2 = 100;
const int MIN_HR = 40;
const int MAX_HR = 250;

int SamplingPeriodMS = REPORTING_PERIOD_MS;
uint32_t tsLastReport = 0;

const uint8_t Duration = 30; //[samplings]
uint8_t t_counter = 0;
uint8_t spo2[Duration]={0};
float heartRate[Duration]={0};
bool buffer_filled = false;

PulseOximeter pox;

// カウンタ管理・全部のバッファが埋まるとbuffer_filledがtrueになる。
uint8_t getNewCounter(){
  uint8_t counter = t_counter;
  t_counter++;
  if(t_counter>=Duration){
    t_counter = 0;
    buffer_filled = true;
  }
  return counter;
}

void resetBuffer(){
  t_counter = 0;
  memset(spo2, 0x00, Duration);
  memset(heartRate, 0x00, sizeof(float)*Duration);
  buffer_filled = false;
  SamplingPeriodMS = REPORTING_PERIOD_MS;

  // センサー再起動
  while(!pox.begin()){
    vTaskDelay(1000); // delay
  }
}

// Callback (registered below) fired when a pulse is detected
void onBeatDetected()
{
    Serial.println("onBeatDetected!");
    M5.Lcd.fillRect(LCD_WIDTH*0.9, LCD_HEIGHT-10, LCD_WIDTH, 10, RED);
    vTaskDelay(100);
    M5.Lcd.fillRect(LCD_WIDTH*0.9, LCD_HEIGHT-10, LCD_WIDTH, 10, BLACK);
}

void setup()
{
    M5.begin();
    M5.Lcd.setRotation(1);
    M5.Lcd.setCursor(0, 40);

    Serial.begin(115200);

    while(!pox.begin()){
      vTaskDelay(1000); // delay
      Serial.println("Initializing pulse oximeter..");
    }

    pox.setIRLedCurrent(MAX30100_LED_CURR_24MA);
    pox.setOnBeatDetectedCallback(onBeatDetected);
}

float aveSpO2(){
    float aveSPO2 = 0;
    for(int i=0; i<Duration; i++){
      aveSPO2 += spo2[i];
    }
    aveSPO2 /= Duration;
    return aveSPO2;
}

float stdSpO2(){
    float ave = aveSpO2();
    float sumDiff2 = 0, diff=0; 
    for(int i=0; i<Duration; i++){
      diff = (spo2[i]-ave);
      sumDiff2 += diff*diff;
    }
    float std = sumDiff2/(Duration-1);
    std = sqrt(std);
    return std;
}

float aveHR(){
    float aveHR = 0;
    for(int i=0; i<Duration; i++){
      aveHR += heartRate[i];
    }
    aveHR /= Duration;
    return aveHR;
}

float stdHR(){
    float ave = aveHR();
    float sumDiff2 = 0, diff=0; 
    for(int i=0; i<Duration; i++){
      diff = (heartRate[i]-ave);
      sumDiff2 += diff*diff;
    }
    float std = sumDiff2/(Duration-1);
    std = sqrt(std);
    return std;
}

bool refineData()
{  
  // データを解析
  float stdHRv = stdHR();
  float aveHRv = aveHR();
  float stdSpO2v = stdSpO2();
  float aveSpO2v = aveSpO2();
  Serial.print("aveHRv:");
  Serial.print(aveHRv);
  Serial.print(", stdHRv:");
  Serial.print(stdHRv);
  Serial.print(", aveSpO2v:");
  Serial.print(aveSpO2v);
  Serial.print(", stdSpO2v:");
  Serial.print(stdSpO2v);
  
  // 外れ値を探す
  int8_t validMaxIdxDst = 0;
  if(stdSpO2v>10 || stdHRv>30){
    // すべて外れ値　→ 再測定
    validMaxIdxDst = 0;
  }else{
    int8_t validIdx[Duration]={-1};
    for(int i=0; i<Duration; i++){
      if( (abs(heartRate[i]-aveHRv)<=stdHRv*THRESH_MUL)
         && (abs((float)spo2[i]-aveSpO2v)<=stdSpO2v*THRESH_MUL) ){
          validIdx[i] = i;
          // Serial.print("valid, ");
       }else{
          validIdx[i] = -1;      
          Serial.print("invalid:");
          Serial.print(i);
          Serial.print(", ");
       }
    }
    Serial.println("");
    
    // 外れ値を除去して配列を詰める
    int8_t validMaxIdxSrc = 0;
    bool finishMoving = false;
    for(int i=0; i<Duration; i++){
      finishMoving = false;
      // 有効なデータを探す
      for(int j=validMaxIdxSrc; j<Duration; j++){
        if(validIdx[j]>=0){
          // 有効なデータが見つかったのでコピー
          heartRate[i] = heartRate[j];
          spo2[i] = spo2[j];
          validMaxIdxDst = i;
          if(validMaxIdxSrc<Duration-1){
            // 次の候補データ
            validMaxIdxSrc = j+1;          
          }else{
            finishMoving = true;
          }
          break;//j
        }else{
          // 有効なデータを探す
          continue;
        }
      }
      if(finishMoving){
        // この先使えるデータはない
        break;
      }
    }
  }
  t_counter = validMaxIdxDst;
  Serial.print("t_counter:");
  Serial.println(t_counter);

  bool bRefined = (t_counter == Duration-1);;
  if(!bRefined){
    buffer_filled = false;
  }
  return bRefined;
}

void loop()
{
    // Make sure to call update as fast as possible
    pox.update();

    // Asynchronously dump heart rate and oxidation levels to the serial
    // For both, a value of 0 means "invalid"
    if (millis() - tsLastReport > SamplingPeriodMS) {
      if(buffer_filled && refineData() ){
        // 測定終了 結果表示
        pox.shutdown();
        SamplingPeriodMS = 1000000; //リフレッシュ間隔を長くする
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setTextSize(2);
        M5.Lcd.setCursor(10, 0);
        M5.Lcd.setTextColor(WHITE);
        M5.Lcd.printf("Result");
        float aveHRv = aveHR();
        float aveSpO2v = aveSpO2();
        if(aveSpO2v>95){
          M5.Lcd.setTextColor(GREEN);
        }else{
          // SpO2が異常値（肺機能障害疑い、90%以下は呼吸不全）
          M5.Lcd.setTextColor(RED);
        }
        M5.Lcd.setCursor(0, 20);
        M5.Lcd.printf("SpO2: %.01f %%", aveSpO2v);      
        M5.Lcd.setTextSize(1);
        M5.Lcd.setCursor(20, 37);
        M5.Lcd.printf("SpO2 STD: %.02f", stdSpO2());      
        M5.Lcd.setTextSize(2);
        M5.Lcd.setCursor(0, 45);
        M5.Lcd.printf("HR: %.01f bpm", aveHRv);
        M5.Lcd.setTextSize(1);
        M5.Lcd.setCursor(20, 62);
        M5.Lcd.printf("HR STD: %.02f", stdHR());      

        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(WHITE);
        M5.Lcd.setCursor(5, 70);
        M5.Lcd.printf("Press M5 btn to restart.");      
      }else{
        // 測定中
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setTextSize(2);

        uint8_t tmp_spo2 = pox.getSpO2();
        float tmp_heartRate = pox.getHeartRate();
        Serial.print("Heart rate:");
        Serial.print(tmp_heartRate);
        Serial.print("bpm / SpO2:");
        Serial.print(tmp_spo2);
        Serial.println("%");

        if(tmp_heartRate<MIN_HR || tmp_heartRate>MAX_HR
          || tmp_spo2<MIN_SPO2 || tmp_spo2>MAX_SPO2){
          M5.Lcd.setTextColor(WHITE);
          M5.Lcd.setCursor(10, 10);
          M5.Lcd.printf("Measuring...");
          M5.Lcd.setCursor(5, 40);
          M5.Lcd.setTextSize(1.5);
          M5.Lcd.printf("Put your finger quietly!");
        }else{
          // 測定値保存
          uint8_t cidx = getNewCounter();
          spo2[cidx] = tmp_spo2;
          heartRate[cidx] = tmp_heartRate;

          if(tmp_spo2>95){
            M5.Lcd.setTextColor(GREEN);
          }else{
            // 正常に測定できていそうだけど、SpO2が異常値（肺機能障害疑い、90%以下は呼吸不全）
            M5.Lcd.setTextColor(RED);
          }
          M5.Lcd.setCursor(0, 10);
          M5.Lcd.printf("SpO2: %d %%", tmp_spo2);
          M5.Lcd.setCursor(0, 35);
          M5.Lcd.printf("HR: %.01f bpm", tmp_heartRate);
        }
        // プログレスバー
        M5.Lcd.fillRect(0, LCD_HEIGHT-10, t_counter*LCD_WIDTH*0.8/Duration, 10, GREEN);
      }
      tsLastReport = millis();
    }
    M5.update();
    if(M5.BtnA.wasPressed()){
      resetBuffer();
    }
 }
 
