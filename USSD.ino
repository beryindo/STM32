#define DEBUG_PORT Serial
char latBuffer[15];
char longBuffer[15];
char koordinat[60];
char buffer[512];

#define DEFAULT_TIMEOUT          5   //seconds
#define Sim800LSerial Serial1 //A2,A3
enum DataType {
  CMD     = 0,
  DATA    = 1,
};

#define MESSAGE_LENGTH 160
char message[MESSAGE_LENGTH];
int messageIndex = 0;

char phone[16];
char datetime[24];
String bacaSMS = "";
char responUSSD[] = "";
char NoHPOK[16];

void setup()
{
  DEBUG_PORT.begin(9600);
  Sim800LSerial.begin(9600);
  DEBUG_PORT.println("sim OK");
  gpsPort.println("gpsPort");
//  Sim800LSerial.println("Sim800LSerial");
  disconnect();
  //  sim900_check_with_cmd(F("AT+CIPCLOSE\r\n"), "CLOSE OK\r\n", CMD, 5000, 15000);
  //  sim900_check_with_cmd(F("AT+CIPSHUT\r\n"), "SHUT OK\r\n", CMD, 5000, 15000);
  while (!cekModuleSim800L()) {
    DEBUG_PORT.print("error euy\r\n");
    delay(1000);
  }
//  delay(20000);
  DEBUG_PORT.println("sip mang, module sim jalan!");
  sim900_check_with_cmd("AT+CMGF=1\r\n", "OK", CMD, 5000, 3000);
  sim900_check_with_cmd("AT+CNMI=2,1,0,0,0\r\n", "OK", CMD, 5000, 3000);
  sim900_check_with_cmd("AT+CPMS=\"SM\",\"SM\",\"SM\"\r\n", "OK", CMD, 5000, 3000);
  sim900_check_with_cmd("AT+CMGD=1,4\r\n", "OK", CMD, 5000, 3000);
  DEBUG_PORT.println("eksekusi CMD selesai");
  Sim800LSerial.println("AT+CUSD=1,\"*888#\"");
  lihat();
  sendUSSD("*888#", "OK", responUSSD);
  DEBUG_PORT.println(responUSSD);
}

void loop() {
 
}

String pecah(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

int sim900_check_readable()
{
  return Sim800LSerial.available();
}

int sim900_wait_readable (int wait_time)
{
  unsigned long timerStart;
  int dataLen = 0;
  timerStart = millis();
  while ((unsigned long) (millis() - timerStart) > wait_time * 1000UL) {
    delay(500);
    dataLen = sim900_check_readable();
    if (dataLen > 0) {
      break;
    }
  }
  return dataLen;
}

void sim900_flush_serial()
{
  while (sim900_check_readable()) {
    Sim800LSerial.read();
  }
}

void sim900_read_buffer(char *buffer, int count, unsigned int timeout, unsigned int chartimeout)
{
  int i = 0;
  unsigned long timerStart, prevChar;
  timerStart = millis();
  prevChar = 0;
  while (1) {
    while (sim900_check_readable()) {
      char c = Sim800LSerial.read();
      prevChar = millis();
      buffer[i++] = c;
      if (i >= count)break;
    }
    if (i >= count)break;
    if ((unsigned long) (millis() - timerStart) > timeout * 1000UL) {
      break;
    }
    //If interchar Timeout => return FALSE. So we can return sooner from this function. Not DO it if we dont recieve at least one char (prevChar <> 0)
    if (((unsigned long) (millis() - prevChar) > chartimeout) && (prevChar != 0)) {
      break;
    }
  }
}

void sim900_clean_buffer(char *buffer, int count)
{
  for (int i = 0; i < count; i++) {
    buffer[i] = '\0';
  }
}

//HACERR quitar esta funcion ?
void sim900_send_byte(uint8_t data)
{
  Sim800LSerial.write(data);
}

void sim900_send_char(const char c)
{
  Sim800LSerial.write(c);
}

void sim900_send_cmd(const char* cmd)
{
  for (uint16_t i = 0; i < strlen(cmd); i++)
  {
    sim900_send_byte(cmd[i]);
  }
}

void sim900_send_cmd(const __FlashStringHelper * cmd)
{
  int i = 0;
  const char *ptr = (const char *) cmd;
  while (pgm_read_byte(ptr + i) != 0x00) {
    sim900_send_byte(pgm_read_byte(ptr + i++));
  }
}

void sim900_send_cmd_P(const char* cmd)
{
  while (pgm_read_byte(cmd) != 0x00)
    sim900_send_byte(pgm_read_byte(cmd++));
}

//boolean sim900_send_AT(void)
//{
//    return sim900_check_with_cmd(F("AT\r\n"),"OK",CMD);
//}

void sim900_send_End_Mark(void)
{
  sim900_send_byte((char)26);
}

boolean sim900_wait_for_resp(const char* resp, DataType type, unsigned int timeout, unsigned int chartimeout)
{
  int len = strlen(resp);
  int sum = 0;
  unsigned long timerStart, prevChar;    //prevChar is the time when the previous Char has been read.
  timerStart = millis();
  prevChar = 0;
  while (1) {
    if (sim900_check_readable()) {
      char c = Sim800LSerial.read();
      prevChar = millis();
      sum = (c == resp[sum]) ? sum + 1 : 0;
      if (sum == len)break;
    }
    if ((unsigned long) (millis() - timerStart) > timeout * 1000UL) {
      return false;
    }
    //If interchar Timeout => return FALSE. So we can return sooner from this function.
    if (((unsigned long) (millis() - prevChar) > chartimeout) && (prevChar != 0)) {
      return false;
    }

  }
  //If is a CMD, we will finish to read buffer.
  if (type == CMD) sim900_flush_serial();
  return true;
}


boolean sim900_check_with_cmd(const char* cmd, const char *resp, DataType type, unsigned int timeout, unsigned int chartimeout)
{
  sim900_send_cmd(cmd);
  return sim900_wait_for_resp(resp, type, timeout, chartimeout);
}

//HACERR que tambien la respuesta pueda ser FLASH STRING
boolean sim900_check_with_cmd(const __FlashStringHelper * cmd, const char *resp, DataType type, unsigned int timeout, unsigned int chartimeout)
{
  sim900_send_cmd(cmd);
  return sim900_wait_for_resp(resp, type, timeout, chartimeout);
}

bool sendUSSD(char *ussdCommand, char *resultcode, char *response)
{
  byte i = 0;
  char gprsBuffer[200];
  char *p, *s;
  sim900_clean_buffer(response, sizeof(response));

  sim900_flush_serial();
  sim900_send_cmd(F("AT+CUSD=1,\""));
  sim900_send_cmd(ussdCommand);
  sim900_send_cmd(F("\"\r"));
  //    boolean sim900_wait_for_resp(const char* resp, DataType type, unsigned int timeout, unsigned int chartimeout)
  if (!sim900_wait_for_resp("OK\r\n", CMD, 5000, 5000))
    return false;
  sim900_clean_buffer(gprsBuffer, 200);
  sim900_read_buffer(gprsBuffer, 200, 5000, 3000);
  if (NULL != ( s = strstr(gprsBuffer, "+CUSD: "))) {
    *resultcode = *(s + 7);
    resultcode[1] = '\0';
    if (!('0' <= *resultcode && *resultcode <= '2'))
      return false;
    s = strstr(s, "\"");
    s = s + 1;  //We are in the first phone number character
    p = strstr(s, "\""); //p is last character """
    if (NULL != s) {
      i = 0;
      while (s < p) {
        response[i++] = *(s++);
      }
      response[i] = '\0';
    }
    return true;
  }
  return false;
  sim900_check_with_cmd("AT+CUSD=2\r\n", "OK", CMD, 5000, 15000);
}

char isSMSunread()
{
  char gprsBuffer[48];  //48 is enough to see +CMGL:
  char *s;

  //List of all UNREAD SMS and DON'T change the SMS UNREAD STATUS
  sim900_send_cmd(F("AT+CMGL=\"REC UNREAD\",1\r\n"));
  sim900_clean_buffer(gprsBuffer, 31);
  sim900_read_buffer(gprsBuffer, 30, 5000, 3000);
  //  Serial.print("Buffer isSMSunread: ");
  //  Serial.println(gprsBuffer);

  if (NULL != ( s = strstr(gprsBuffer, "OK"))) {
    //In 30 bytes "doesn't" fit whole +CMGL: response, if recieve only "OK"
    //    means you don't have any UNREAD SMS
    delay(50);
    return 0;
  } else {
    //More buffer to read
    //We are going to flush serial data until OK is recieved
    sim900_wait_for_resp("OK\r\n", CMD, 5000, 3000);
    //    sim900_flush_serial();
    //We have to call command again
    sim900_send_cmd(F("AT+CMGL=\"REC UNREAD\",1\r\n"));
    sim900_clean_buffer(gprsBuffer, 48);
    sim900_read_buffer(gprsBuffer, 47, 5000, 3000);
    //    Serial.print("Buffer isSMSunread 2: ");
    //    Serial.println(gprsBuffer);
    if (NULL != ( s = strstr(gprsBuffer, "+CMGL:"))) {
      //There is at least one UNREAD SMS, get index/position
      s = strstr(gprsBuffer, ":");
      //      Serial.println(s);
      if (s != NULL) {
        //We are going to flush serial data until OK is recieved
        sim900_wait_for_resp("OK\r\n", CMD, 5000, 3000);
        return atoi(s + 1);
      }
    } else {
      return -1;
    }
  }
  return -1;
}

bool readSMS(int messageIndex, char *message, int length, char *phone, char *datetime)
{
  int i = 0;
  char gprsBuffer[80 + length];
  //  char cmd[16];
  char num[4];
  char *p, *p2, *s;

  sim900_check_with_cmd(F("AT+CMGF=1\r\n"), "OK\r\n", CMD, 5000, 3000);
  delay(1000);
  //  sprintf(cmd, "AT+CMGR=%d\r\n", messageIndex);
  //  Serial.println(cmd);
  //sim900_send_cmd(cmd);
  sim900_send_cmd(F("AT+CMGR="));
  //  itoa(messageIndex, num, 10);
  sprintf(num, "%d", messageIndex);
  sim900_send_cmd(num);
  sim900_send_cmd(F("\r\n"));
  sim900_clean_buffer(gprsBuffer, sizeof(gprsBuffer));
  sim900_read_buffer(gprsBuffer, sizeof(gprsBuffer), 5000, 3000);

  if (NULL != ( s = strstr(gprsBuffer, "+CMGR:"))) {
    // Extract phone number string
    p = strstr(s, ",");
    p2 = p + 2; //We are in the first phone number character
    p = strstr((char *)(p2), "\"");
    if (NULL != p) {
      i = 0;
      while (p2 < p) {
        phone[i++] = *(p2++);
      }
      phone[i] = '\0';
    }
    // Extract date time string
    p = strstr((char *)(p2), ",");
    p2 = p + 1;
    p = strstr((char *)(p2), ",");
    p2 = p + 2; //We are in the first date time character
    p = strstr((char *)(p2), "\"");
    if (NULL != p) {
      i = 0;
      while (p2 < p) {
        datetime[i++] = *(p2++);
      }
      datetime[i] = '\0';
    }
    if (NULL != ( s = strstr(s, "\r\n"))) {
      i = 0;
      p = s + 2;
      while ((*p != '\r') && (i < length - 1)) {
        message[i++] = *(p++);
        //        Serial.println(message);
      }
      message[i] = '\0';
    }
    return true;
  }
  return false;
}

bool readSMS(int messageIndex, char *message, int length)
{
  int i = 0;
  char gprsBuffer[100];
  //char cmd[16];
  char num[4];
  char *p, *s;

  sim900_check_with_cmd(F("AT+CMGF=1\r\n"), "OK\r\n", CMD, 5000, 3000);
  delay(1000);
  sim900_send_cmd(F("AT+CMGR="));
  sprintf(num, "%d", messageIndex);
  //  itoa(messageIndex, num, 10);
  sim900_send_cmd(num);
  sim900_send_cmd(F("\r\n"));
  //  sprintf(cmd,"AT+CMGR=%d\r\n",messageIndex);
  //    sim900_send_cmd(cmd);
  sim900_clean_buffer(gprsBuffer, sizeof(gprsBuffer));
  sim900_read_buffer(gprsBuffer, sizeof(gprsBuffer), 5000, 3000);
  if (NULL != ( s = strstr(gprsBuffer, "+CMGR:"))) {
    if (NULL != ( s = strstr(s, "\r\n"))) {
      p = s + 2;
      while ((*p != '\r') && (i < length - 1)) {
        message[i++] = *(p++);
      }
      message[i] = '\0';
      return true;
    }
  }
  return false;
}

bool sendSMS(char *number, char *data)
{
  if (!sim900_check_with_cmd(F("AT+CMGF=1\r\n"), "OK\r\n", CMD, 5000, 3000)) { // Set message mode to ASCII
    return false;
  }
  delay(500);
  sim900_flush_serial();
  sim900_send_cmd(F("AT+CMGS=\""));
  sim900_send_cmd(number);
  if (!sim900_check_with_cmd(F("\"\r\n"), ">", CMD, 5000, 3000)) {
    return false;
  }
  delay(1000);
  sim900_send_cmd(data);
  delay(500);
  sim900_send_End_Mark();
  return sim900_wait_for_resp("OK\r\n", CMD, DEFAULT_TIMEOUT, DEFAULT_TIMEOUT);
}

bool deleteSMS(int index)
{
  char num[4];
  sim900_send_cmd(F("AT+CMGD="));
  sprintf(num, "%d", index);
  //  itoa(index, num, 10);
  sim900_send_cmd(num);
  return sim900_check_with_cmd(F("\r"), "OK\r\n", CMD, 5000, 3000);
}

bool cekModuleSim800L(void)
{
  if (!sim900_check_with_cmd(F("AT\r\n"), "OK\r\n", CMD, 5000, 3000)) {
    return false;
  }

  if (!sim900_check_with_cmd(F("AT+CFUN=1\r\n"), "OK\r\n", CMD, 5000, 3000)) {
    return false;
  }

  if (!checkSIMStatus()) {
    return false;
  }
  return true;
}

bool checkSIMStatus(void)
{
  char gprsBuffer[32];
  int count = 0;
  sim900_clean_buffer(gprsBuffer, 32);
  while (count < 3) {
    sim900_send_cmd(F("AT+CPIN?\r\n"));
    sim900_read_buffer(gprsBuffer, 32, 5000, 3000);
    if ((NULL != strstr(gprsBuffer, "+CPIN: READY"))) {
      break;
    }
    count++;
    delay(300);
  }
  if (count == 3) {
    return false;
  }
  return true;
}

void disconnect()
{
  sim900_check_with_cmd(F("AT+CIPSHUT\r\n"), "SHUT OK\r\n", CMD, 3000, 15000);
}

bool getSignalStrength(int *buffer)
{
  //AT+CSQ            --> 6 + CR = 10
  //+CSQ: <rssi>,<ber>      --> CRLF + 5 + CRLF = 9
  //OK              --> CRLF + 2 + CRLF =  6

  byte i = 0;
  char gprsBuffer[26];
  char *p, *s;
  char buffers[4];
  sim900_flush_serial();
  sim900_send_cmd(F("AT+CSQ\r"));
  sim900_clean_buffer(gprsBuffer, 26);
  sim900_read_buffer(gprsBuffer, 26, 5000, 3000);
  if (NULL != (s = strstr(gprsBuffer, "+CSQ:"))) {
    s = strstr((char *)(s), " ");
    s = s + 1;  //We are in the first phone number character
    p = strstr((char *)(s), ","); //p is last character """
    if (NULL != s) {
      i = 0;
      while (s < p) {
        buffers[i++] = *(s++);
      }
      buffers[i] = '\0';
    }
    *buffer = atoi(buffers);
    return true;
  }
}

void resetSIM800L(uint8_t pin)
{
  // power on pulse for SIM900 Shield
  digitalWrite(pin, LOW);
  delay(1000);
  digitalWrite(pin, HIGH);
  delay(2000);
  digitalWrite(pin, LOW);
  delay(3000);
}

void lihat() {
  if (Sim800LSerial.available()) {
    DEBUG_PORT.write(Sim800LSerial.read());
  }
  if (DEBUG_PORT.available()) {
    Sim800LSerial.write(DEBUG_PORT.read());
  }
}

void printSerialData()
{
  while (Sim800LSerial.available() != 0)
    DEBUG_PORT.write(Sim800LSerial.read());
}
