/**
 * 因为SIM800太费电，需要的电流太大，本地测试困难，我回归本真，先来个，只记录到SD卡里的
 * 
 */
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <SdFat.h>
#include <dht11.h>

#define PIN_GPS_PPS 2
#define PIN_DHT11 7
#define PIN_SD_CS 10

SoftwareSerial gps(5,6);

SdFat sd;
SdFile dataFile;

dht11 DHT11;

//每一次文件id
unsigned long fileId=0;

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

  Serial.setTimeout(5000);
  gps.setTimeout(5000);

  //初始化文件id序号
  EEPROM.get(0, fileId);
  //加一个
  fileId++;
  //再保存
  EEPROM.put(0, fileId);
  
  //初始化SD卡
  if (!sd.begin(PIN_SD_CS, SPI_FULL_SPEED)){
    sd.initErrorHalt();
  }
  
  //打开文件
  String fileName=String(fileId)+".txt";
  char fileNameArray[fileName.length()+1];
  fileName.toCharArray(fileNameArray,fileName.length()+1);
  fileNameArray[fileName.length()]='\0';
  if (!dataFile.open(fileNameArray, O_RDWR | O_CREAT | O_AT_END)) {
    sd.errorHalt("sd fail");
  }
}

void loop() {
  if(gps.available()){
    while (gps.available()>0) {
      //读GPS数据
      String gpsLine=gps.readStringUntil('\n');
      //保存到SD卡
      dataFile.println(gpsLine);
      delay(5);
    }
    //保存DHT11温湿度信息
    DHT11.read(PIN_DHT11);
    //温度
    int tem=(float)DHT11.temperature;
    //湿度
    int hum=(float)DHT11.humidity;
    //保存数据
    dataFile.print("#####temperature=");
    dataFile.print(tem);
    dataFile.print("&humidity=");
    dataFile.print(hum);
    dataFile.println("#####");
    //刷新SD卡
    dataFile.flush();
    delay(10);
  }
}
