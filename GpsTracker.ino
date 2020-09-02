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
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <SdFat.h>
//#include "U8glib.h"
#include <dht11.h>
//#include <Chrono.h>

#define PIN_GPS_PPS 2
#define PIN_DHT11 7
#define PIN_SD_CS 10
#define PIN_SIM800_RESET 8

SoftwareSerial gps(5,6);
SoftwareSerial sim(3,4);

SdFat sd;
SdFile dataFile;
String gpsLine;

//U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0);
dht11 DHT11;

//显示屏缓存string数组
//String oledLines[5];

//Chrono oledChrono;

//每一次文件id
unsigned long fileId=0;
unsigned int fileIdLength=0;

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
  sim.setTimeout(15000);

  //初始化文件id序号
  EEPROM.get(0, fileId);
  //加一个
  fileId++;
  //再保存
  EEPROM.put(0, fileId);
  //计算数字长度
  if(fileId<=9){
    fileIdLength=1;
  }else if(fileIdLength<=99){
    fileIdLength=2;
  }else if(fileIdLength<=999){
    fileIdLength=3;
  }else if(fileIdLength<=9999){
    fileIdLength=4;
  }else if(fileIdLength<=99999){
    fileIdLength=5;
  }else if(fileIdLength<=999999){
    fileIdLength=6;
  }else if(fileIdLength<=9999999){
    fileIdLength=7;
  }
  
  
  //初始化显示屏
//  u8g.setFont(u8g_font_unifont);
//  for(int i=0;i<5;i++){
//    oledLines[i]="";
//  }
  
  //初始化SD卡
  if (!sd.begin(PIN_SD_CS, SPI_FULL_SPEED)){
    sd.initErrorHalt();
//    oledLines[3]=F("sd begin fail");
//    refreshOled();
    delay(1000);
  }
  //打开文件
  String fileName=String(fileId)+".txt";
  char fileNameArray[fileName.length()+1];
  fileName.toCharArray(fileNameArray,fileName.length()+1);
  fileNameArray[fileName.length()]='\0';
  if (!dataFile.open(fileNameArray, O_RDWR | O_CREAT | O_AT_END)) {
    sd.errorHalt("sd fail");
//    oledLines[4]=F("open file fail");
//    refreshOled();
    delay(1000);
  }
  dataFile.println("teaae");
  dataFile.flush();

  //初始化SIM800
  Serial.println("start to wait");
  delay(4000);
  sendSim800("AT","OK",500);//模块开机测试
  sendSim800("AT+CPIN?","+CPIN: READY",500);//插卡
//  sendSim800("AT+CSQ","OK",500);//查询信号质量
  //等待注册到网络
  int count=0;
  while(true){
    count++;
    if(count>=8){
      break;
    }
    int result=sendSim800("AT+CREG?","+CREG: 0,1",500);
    delay(500);
    if(result==0){
      result=sendSim800("AT+CREG?","+CREG: 0,5",500);
      if(result==1){
        break;
      }
    }else if(result==1){
      break;
    }
    delay(1000);
  }
  sendSim800("AT+CGATT?","+CGATT: 1",1000);//检查 GPRS 附着状态
  sendSim800("AT+CSTT=\"CMNET\"","OK",2000);//设置APN
  sendSim800("AT+CIICR","OK",3000);//建立无线链路 (GPRS 或者 CSD)
  sendSim800("AT+CIFSR","OK",2000);//获得本地 IP 地址
  
  //连接状态查询：AT+CIPSTATUS   返回 STATE: CONNECT OK
  sendSim800("AT+CIPSTATUS","STATE: CONNECT OK",1000);

  
  //建立 TCP 链接
  //AT+CIPSTART="TCP","106.12.57.49","8500"
  //AT+CIPSTART="TCP","c19758058n.imwork.net","40324"
  sendSim800("AT+CIPSTART=\"TCP\",\"106.12.57.49\",\"8500\"","\"CONNECT OK\"",5000);
//  sendSim800("AT+CIPSTART=\"TCP\",\"c19758058n.imwork.net\",\"40324\"","\"CONNECT OK\"",5000);
  //连接状态查询：AT+CIPSTATUS   返回 STATE: CONNECT OK
  sendSim800("AT+CIPSTATUS","STATE: CONNECT OK",1000);


    String cmd="AT+CIPSEND=3";
    sim.println(cmd);
    delay(800);
    sim.println("abc");
    delay(800);
}

void loop() {
  gps.listen();
  //等到一个数据的开始
  while(gps.available()>0){
    while(!gps.find("$GNGGA")){
      ;
    }
    //处理第一行的开头从buffer中丢失的头
    gpsLine=gps.readStringUntil('\n');
    handleGpsLine("$GNGGA"+gpsLine);
    break;
  }
  
  //读数据
  while (gps.available()>0) {
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
//    oledChrono.restart();
//    refreshOled();
//  }
}

/**
 * 发给SIM800命令
 * 返回1，成功
 * 返回0，失败
 * 返回-1，超时
 */
int sendSim800(String cmd,String until,long long wait){
  delay(500);
  sim.listen();
  sim.println(cmd);
  long long startMillis=millis();
  //一直等到SIM800有数据返回
  while(sim.available()<=0){
    //等待模块返回数据
    Serial.println("wait");
    delay(wait);
    //超时判断
    if((millis()-startMillis)>15000){
      return -1;
    }
  }
  //这里就是有数据了
  while(sim.available()>0){
    String line=sim.readStringUntil('\r');
    Serial.print(line);
    //找到要找的了，返回成功
    if(line.indexOf(until)>=0){
      //把缓存剩下的也读完
      while(sim.available()>0){
        sim.read();
      }
      return 1;
    }
  }
  //失败
  return 0;
}

/**
 * 刷新显示屏
 */
//void refreshOled(){
//  u8g.firstPage();
//  do {
//    if(oledLines[0]!=""){
//      u8g.setPrintPos(0, 11);
//      u8g.print(oledLines[0]);
//    }
//    if(oledLines[1]!=""){
//      u8g.setPrintPos(0, 24);
//      u8g.print(oledLines[1]);
//    }
//    if(oledLines[2]!=""){
//      u8g.setPrintPos(0, 37);
//      u8g.print(oledLines[2]);
//    }
//    if(oledLines[3]!=""){
//      u8g.setPrintPos(0, 50);
//      u8g.print(oledLines[3]);
//    }
//    if(oledLines[4]!=""){
//      u8g.setPrintPos(0, 63);
//      u8g.print(oledLines[4]);
//    }
//  } while( u8g.nextPage() ); 
//}

String GNRMC;
/**
 * 处理GPS的一行数据
 */
void handleGpsLine(String singleLine){
  //如果是GNRMC，我们先记录下来，等到最后一个GNTXT，再处理，否则会错过后面的串口输出
  if(singleLine.startsWith("$GNRMC")){
    GNRMC=singleLine;
  }else if(singleLine.startsWith("$GPTXT")){
    /**
     *  AT+CIPSTART="TCP","c19758058n.imwork.net","40324"
        AT+CIPSTATUS
        AT+CIPSEND=4
        AT+CIPCLOSE
     */
    sim.listen();

    //更新温湿度
    int chk=DHT11.read(PIN_DHT11);
    int tem=(float)DHT11.temperature;
    int hum=(float)DHT11.humidity;
    Serial.println(tem);
    
    int sendLength=14+fileIdLength+GNRMC.length();
    String cmd="AT+CIPSEND="+String(sendLength);
    sim.println(cmd);
    Serial.println(GNRMC);
    delay(500);
    sim.print("fileId=");
    sim.print(String(fileId));
    sim.print("&GNRMC=");
    sim.println(GNRMC);
    delay(100);
    //如果是最后一行GNTXT，现在就有时间处理数据了
    //把字符串用逗号分割成数组
//    String splitArray[14];
//    for(int i=0;i<14;i++){
//      splitArray[i]="";
//    }
//    //数组中的索引
//    int arrayIndex=0;
//    for(int i=0;i<GNRMC.length();i++){
//      char c=GNRMC.charAt(i);
//      //如果是有用的数据，保存下来
//      if(c!=','){
//        splitArray[arrayIndex]=splitArray[arrayIndex]+c;
//      }else{
//        //如果是逗号，索引加1
//        arrayIndex++;
//      }
//    }


//    String gpsDate=splitArray[9].substring(2,4)+"-"+splitArray[9].substring(4,6);
//    String gpsTime=splitArray[1].substring(0,2)+":"+splitArray[1].substring(2,4)
//        +":"+splitArray[1].substring(4,6);
//    oledLines[0]=splitArray[1];
//    oledLines[1]=splitArray[3];
//    Serial.println(splitArray[1]);
//    oledLines[2]=splitArray[5];
    
//    oledLines[3]=String(tem)+" "+String(hum);
    //刷新显示屏
//    refreshOled();
    GNRMC="";
  }
}
