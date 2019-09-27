#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <SoftwareSerial.h> 
#include <OneWire.h>
#include <DallasTemperature.h>
#include <avr/wdt.h>  
#include <avr/sleep.h>

//定义DS18B20引脚，初始化对象
#define BUS 4
OneWire onewire(BUS);
DallasTemperature sensors(&onewire);

//定义DHT11引脚号
#define pin 2

//定义I2C地址0x27，规模1602。
LiquidCrystal_I2C lcd(0x27,16,2);

// Pin10为RX，接HC05的TXD
// Pin11为TX，接HC05的RXD
SoftwareSerial BT(10, 11); 

int humi;//定义湿度
int veri;//定义校对码
int veri2;
int temp;//定义温度
int j;//定义变量
int shu = 0;
unsigned int loopCnt;
int chr[40] = {0};//创建数字数组，用来存放40个bit
unsigned long time;

ISR(WDT_vect){
  //看门狗唤醒执行函数
  shu++;
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  
  sensors.begin();
  Serial.begin(9600);//设置串口波特率38400
  BT.begin(9600);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); //设置休眠模式。
  //开始设置看门狗中断，用来唤醒。   
  MCUSR &= ~(1<<WDRF);
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  WDTCSR = 1<<WDP1 | 1<<WDP2;
  WDTCSR |= _BV(WDIE); 

  lcd.init();
  lcd.backlight();
  lcd.print("temp:");
   lcd.setCursor(7,0);
  lcd.print(";");
  lcd.setCursor(14,0);//
  lcd.print((char)223); 
  lcd.println("C"); //显示摄氏度符号
  lcd.setCursor(0,1);//在LCD上换行
  lcd.print("humi:");
  lcd.setCursor(11,1);//
  lcd.println("  %RH");//空两行防止出现乱码，同时将湿度单位输出到LCD

}

void loop()
{   
  while(init_all() != 1);
  if (shu>=2){//喂狗间隔
    read_data();
    {
      print_data();  
    }
    shu=0;
  }
  sleep_mode(); //进入休眠状态，从此处开始进入休眠。
  //这里不需要喂狗。目的就是等狗超时后执行唤醒函数。 
   receive_instruction(); 
}



int init_all()
{
  delay(1970);
  pinMode(pin,OUTPUT);
  digitalWrite(pin,LOW);
  delay(20);
  digitalWrite(pin,HIGH);
  delayMicroseconds(40);
  digitalWrite(pin,LOW);
  //设置2号接口模式：输入
  pinMode(pin,INPUT);
  digitalWrite(LED_BUILTIN, LOW);    
  //高电平响应信号
  loopCnt=10000;
  while(digitalRead(pin) != HIGH)
  {
    if(loopCnt-- == 0)
    {
    //如果长时间不返回高电平，输出个提示，重头开始。
      Serial.println("HIGH"); 
      return 0;
    }
  }

  //低电平响应信号
  loopCnt=30000;
  while(digitalRead(pin) != LOW)
  {
    if(loopCnt-- == 0)
    {
    //如果长时间不返回低电平，输出个提示，重头开始。
      Serial.println("LOW");
      return 0;
    }
  }
  return 1;
}


//读取数值 
void read_data()
{
  //读取DHT11
  for(int i=0;i<40;i++)
  {
    while(digitalRead(pin) == LOW)
    {}
    time = micros();
    while(digitalRead(pin) == HIGH)
    {}
    //当出现低电平，记下时间，再减去刚才储存的time
    //得出的值若大于50μs，则为‘1’，否则为‘0’
    //并储存到数组里去
    if (micros() - time >50)
    {
      chr[i]=1;
    }else{
      chr[i]=0;
    }
  }
  //湿度，8位的bit，转换为数值
  humi = chr[0]*128+chr[1]*64+chr[2]*32+chr[3]*16+chr[4]*8+chr[5]*4+chr[6]*2+chr[7];
  //温度，8位的bit，转换为数值
  temp = chr[16]*128+chr[17]*64+chr[18]*32+chr[19]*16+chr[20]*8+chr[21]*4+chr[22]*2+chr[23];
  //校对码，8位的bit，转换为数值
  veri = chr[32]*128+chr[33]*64+chr[34]*32+chr[35]*16+chr[36]*8+chr[37]*4+chr[38]*2+chr[39];
  veri2 =    (chr[4]*8+chr[5]*4+chr[6]*2+chr[7]  +  chr[20]*8+chr[21]*4+chr[22]*2+chr[23])+
          16*(chr[0]*8+chr[1]*4+chr[2]*2+chr[3]  +  chr[16]*8+chr[17]*4+chr[18]*2+chr[19]*16);
          
  //读取DS18B20        
  sensors.requestTemperatures();
}

//输出：温度、湿度、校对码
void print_data()
{
  Serial.print("temperature_1:");
  Serial.print(temp);
  Serial.println("°C");
  Serial.print("humidity:");
  Serial.print(humi);
  Serial.println("%RH");
  Serial.print("verify:");
  Serial.println(veri);
  Serial.write("temperature_2:");
  Serial.print(sensors.getTempCByIndex(0));
  Serial.println("°C");
  
  lcd.setCursor(5,0);//在LCD上换行
  lcd.print(temp);
  lcd.setCursor(8,0);
  lcd.print( sensors.getTempCByIndex(0) );
  lcd.setCursor(5,1);//在LCD上换行
  lcd.print(humi);
}

//接受指令，并执行
void receive_instruction()
{
  while(BT.available())
  {
    char c=BT.read();
    if(c=='1')
    {
      digitalWrite(LED_BUILTIN, HIGH);   
      delay(1000);                       
      digitalWrite(LED_BUILTIN, LOW);  
      BT.println("hi");                 
    }
    if(c=='q'){//t1
      BT.print("temperature_1: ");
      BT.println(temp);
      BT.print("temperature_2: ");
      BT.println(sensors.getTempCByIndex(0));
      BT.print("humidity: ");
      BT.println(humi);
    }

  }
}
