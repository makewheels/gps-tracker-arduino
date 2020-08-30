/**
 * 2020年8月18日20:03:39
 * 
 * 
 * SIM800命令
 * 
 * AT+CPIN?         插卡检测
 * AT+CSQ           信号强度
 * AT+CGATT?        GPRS附着状态
 * AT+CSTT="CMNET"  配置APN
 * AT+CIICR         建立无线链路 (GPRS 或者 CSD)
 * AT+CIFSR         获得本地 IP 地址
 * AT+CIPSTART="TCP","116.228.221.51","8500"    //建立 TCP 链接
 * AT+CIPSEND       用户必须要等到 “>”后才输入数据 , 然后用CTRL+Z 发送
 * 
 * 
 * 
 * AT+CSTT 运营商信息
 * AT+GSN IMEI
 * 
 * 
 * 
    $GNGGA,110052.000,4635.27581,N,12507.97558,E,1,06,7.9,38.2,M,0.0,M,,*4E
    $GNGLL,4635.27581,N,12507.97558,E,110052.000,A,A*4A
    $GNGSA,A,3,15,18,194,,,,,,,,,,8.3,7.9,2.6,1*01
    $GNGSA,A,3,01,41,59,,,,,,,,,,8.3,7.9,2.6,4*3D
    $GNGSA,A,3,,,,,,,,,,,,,8.3,7.9,2.6,2*33
    $GPGSV,3,1,11,10,62,293,,12,26,138,,15,29,093,17,18,11,203,19,0*64
    $GPGSV,3,2,11,20,66,197,,21,26,287,,24,66,066,,25,10,168,,0*6C
    $GPGSV,3,3,11,32,25,280,,193,67,141,,194,47,148,18,0*5C
    $BDGSV,2,1,06,01,33,154,28,23,63,087,,37,45,190,,38,19,200,,0*7D
    $BDGSV,2,2,06,41,18,126,23,59,35,160,24,0*71
    $GLGSV,2,1,07,78,43,043,36,70,22,310,35,86,12,343,,80,17,193,,0*78
    $GLGSV,2,2,07,79,65,146,,69,73,015,,85,16,295,,0*4B
    $GNRMC,110052.000,A,4635.27581,N,12507.97558,E,1.21,183.67,290820,,,A,V*0F
    $GNVTG,183.67,T,,M,1.21,N,2.24,K,A*2E
    $GNZDA,110052.000,29,08,2020,00,00*4C
    $GPTXT,01,01,01,ANTENNA OK*35
 * 
 */
#include <SoftwareSerial.h>
#include <SdFat.h>
#include "U8glib.h"
#include <dht11.h>
#include <Chrono.h>

#define PIN_GPS_PPS 2
#define PIN_DHT11 7
#define PIN_SD_CS 10


SoftwareSerial sim(3,4);//RX,TX
SoftwareSerial gps(5,6);

SdFat sd;
SdFile dataFile;
String gpsLine;

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0);
dht11 DHT11;

//显示屏缓存string数组
String oledLines[5];

Chrono oledChrono;

void setup() {
  //初始化串口
  Serial.begin(115200);
  while (!Serial) {
    ;
  }
  gps.begin(9600);
  while (!gps) {
    ;
  }
  sim.begin(9600);
  while (!sim) {
    ;
  }
  
  Serial.setTimeout(5000);
  gps.setTimeout(5000);
  sim.setTimeout(5000);
  
  gps.listen();
  
  //初始化显示屏
  u8g.setFont(u8g_font_unifont);

  //初始化SD卡
  if (!sd.begin(PIN_SD_CS, SPI_FULL_SPEED)){
    sd.initErrorHalt();
    oledLines[3]=F("sd begin fail");
    refreshOled();
    delay(1000);
  }
  if (!dataFile.open("datalog.txt", O_RDWR | O_CREAT | O_AT_END)) {
    sd.errorHalt("sd fail");
    oledLines[4]=F("open file fail");
    refreshOled();
    delay(1000);
  }
}

void loop() {
  //等到一个数据的开始
  while(gps.available()){
    while(!gps.find("$GNGGA")){
      ;
    }
    //处理第一行的开头从buffer中丢失的头
    gpsLine=gps.readStringUntil('\n');
    handleGpsLine("$GNGGA"+gpsLine);
    break;
  }
  
  //读数据
  while (gps.available()) {
    //读GPS数据
    gpsLine=gps.readStringUntil('\n');
    //处理一行
    handleGpsLine(gpsLine);
    //保存到SD卡
    dataFile.println(gpsLine);
  }
  //刷新SD卡
  dataFile.flush();
 
  
  //多线程，每隔一秒执行一次
//  if (oledChrono.hasPassed(1000)){
////    Serial.println("=====================");
////    Serial.println(GNRMC);
//    oledChrono.restart();
//    refreshOled();
//    dataFile.flush();
//  }
}

/**
 * 刷新显示屏
 */
void refreshOled(){
  u8g.firstPage();
  do {
    u8g.setPrintPos(0, 11);
    u8g.print(oledLines[0]);
    u8g.setPrintPos(0, 24);
    u8g.print(oledLines[1]);
    u8g.setPrintPos(0, 37);
    u8g.print(oledLines[2]);
    u8g.setPrintPos(0, 50);
    u8g.print(oledLines[3]);
    u8g.setPrintPos(0, 63);
    u8g.print(oledLines[4]);
  } while( u8g.nextPage() ); 
}

String GNRMC;
/**
 * 处理GPS的一行数据
 */
void handleGpsLine(String singleLine){
  long long startMillis=millis();
  
  //如果是GNRMC，我们先记录下来，等到最后一个GNTXT，再处理
  //否则会错过后面的串口输出
  //纬度
  String gpsLatitude;
  //经度
  String gpsLongitude;
  String gpsDate;
  String gpsTime;
 
  if(singleLine.startsWith("$GNRMC")){
    GNRMC=singleLine;
  }else if(singleLine.startsWith("$GPTXT")){
    //如果是最后一行GNTXT，现在就有时间处理数据了
    //统计逗号数量
    int commaCount=0;
    //遍历字符串
    for(int i=0;i<GNRMC.length();i++){
      char eachChar=GNRMC.charAt(i);
      //如果遇到逗号
      if(eachChar==','){
        commaCount++;
        //纬度
        if(commaCount==1){
          char nextChar='_';
          while((i+1)<GNRMC.length()&&GNRMC.charAt(i+1)!=','){
            i++;
            nextChar=GNRMC.charAt(i);
            gpsLatitude=gpsLatitude+nextChar;
          }
          Serial.println(gpsLatitude);
        }else if(commaCount==3){
          char nextChar='_';
          while((i+1)<GNRMC.length()&&GNRMC.charAt(i+1)!=','){
            i++;
            nextChar=GNRMC.charAt(i);
            gpsLatitude=gpsLatitude+nextChar;
          }
          Serial.println(gpsLatitude);
        }
      }
    }
    //更新显示屏
    //更新时间
    //更新纬度
    //更新经度
//      oledLines[2]=singleLine.substring(20,30);
    //更新温湿度

    
    //刷新显示屏
//      refreshOled();
    GNRMC="";
  }
    
    int costTime=millis()-startMillis;
//    if(costTime>=2)
//    Serial.println(costTime);
}

//void getDht11(){
//  int chk = DHT11.read(PIN_DHT11);
//  int tem=(float)DHT11.temperature;
//  int hum=(float)DHT11.humidity;
//  
//}
