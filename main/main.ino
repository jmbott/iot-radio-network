
/******************************************* Meter Control *****************************************************/

#define METER_PER_BOX               10

#define REF_DAT_BEGIN 1468886400L
#define ONE_DAY_TIME  86400
#define OVER_CURRENT  1000
#define OVER_CURRENT_PROTECT_TIME 30


#define METER_BOX_NUMBER_ADDR     0
#define METER_BOX_OBJECT_ADDR      METER_BOX_NUMBER_ADDR+1

typedef struct {
  uint32_t amp;
  uint32_t watt;
  uint32_t kwh;
  long credit;
  uint8_t flag;
  uint8_t exc;
} Meter_Type;

#define METER_STATUS_POWER_ON       1
#define METER_STATUS_NO_CREDIT_OFF  2
#define METER_STATUS_SWITCH_OFF     3
#define METER_STATUS_OVER_AMP_OFF   4
#define METER_STATUS_BATTERY_LOW    5

#define CHARGE_CONTROLLOR_VOLTAGE_IDEL     0
#define CHARGE_CONTROLLOR_VOLTAGE_NORMAL   1
#define CHARGE_CONTROLLOR_VOLTAGE_LOW      2
#define CHARGE_CONTROLLOR_VOLTAGE_PROTECT  3
int charge_controller_status = 0;

float cc_voltage = 0.0;

typedef struct {
  int id;
  uint8_t ip[4];
  uint16_t port;
  uint8_t flag;
  uint8_t updated;
  uint8_t error;
  Meter_Type meter_list[METER_PER_BOX];
} Meter_Box_Type;

#define METER_BOX_OFFLINE 0
#define METER_BOX_ONLINE  1

#define METER_DATA_UPDATED 1
#define METER_DATA_INIT    0


#define BOX_TASK_FLAG_STOP  0
#define BOX_TASK_FLAG_RUN   1

#define BOX_TASK_IDLE           0
#define BOX_TASK_CONNECTING     1
#define BOX_TASK_READING        2
#define BOX_TASK_EXCUTE         3
#define BOX_TASK_DISCONNECT     4

char dynamic_memery[sizeof(Meter_Box_Type) * 4];

Meter_Box_Type *meter_box_list;
uint32_t *protect_time_list, *temp_kwh_list;

uint32_t thisMidnight = 0, nextMidnight = 0;
uint16_t meter_box_number = 0;

long current_price = 0;

struct TARIFF {
  uint32_t price1_start_time;
  uint32_t price2_start_time;
  uint16_t price1;
  uint16_t price2;
  uint32_t generate_time;
  uint32_t init_time;
} price_tariff,new_price_tariff;


#define FLASH_MAX_RECORD       2
#define VENDOR_NUMBER          2
#define SN_LENGTH             16
#define PAYMENT_RECORD_NUMBER 50
#define PAYMENT_RECORD_LEN    20

#define SYSTEM_INIT_ADDR              0   //1
#define METER_NUMBER_ADDR             SYSTEM_INIT_ADDR+1    //1
#define MINI_GRID_SN_ADDR             METER_NUMBER_ADDR+1   //16
#define VENDOR_NUMBER_ADDR            MINI_GRID_SN_ADDR+16  //1
#define VENDOR_CREDIT_ADDR            VENDOR_NUMBER_ADDR+1  //8
#define STATIC_IP_ADDR                VENDOR_CREDIT_ADDR+sizeof(long)*VENDOR_NUMBER   //4
#define CHARGE_CONTROLLER_IP_ADDR     STATIC_IP_ADDR+4  // 4
#define BATT_PROTECT_VOLTAGE_ADDR     CHARGE_CONTROLLER_IP_ADDR+4 //2
#define BATT_RECOVER_VOLTAGE_ADDR     BATT_PROTECT_VOLTAGE_ADDR+2 //2
#define TARIFF_ADDR                   BATT_RECOVER_VOLTAGE_ADDR+2 //24
#define NEW_TARIFF_FLAG_ADDR          TARIFF_ADDR+sizeof(price_tariff)  //1
#define NEW_TARIFF_ADDR               NEW_TARIFF_FLAG_ADDR+1  //24
#define TIME_RECORD_LIST_ADDR         NEW_TARIFF_ADDR+sizeof(price_tariff)  //16
//#define PAYMENT_NUMBER_ADDR           TIME_RECORD_LIST_ADDR+sizeof(uint32_t)*FLASH_MAX_RECORD
//#define PAYMENT_INDEX_ADDR            PAYMENT_NUMBER_ADDR+1
//#define PAYMENT_RECORD_ADDR           PAYMENT_INDEX_ADDR+1
//#define METER_RECORD_LIST_ADDR        PAYMENT_RECORD_ADDR+PAYMENT_RECORD_NUMBER*PAYMENT_RECORD_LEN
#define METER_RECORD_LIST_ADDR        (TIME_RECORD_LIST_ADDR+sizeof(uint32_t)*FLASH_MAX_RECORD+3)&0xfffffffc
#define METER_RECORD_LIST_ADDR_END    METER_RECORD_LIST_ADDR+sizeof(Meter_Box_Type)*FLASH_MAX_RECORD

#define PAYMENT_NUMBER_ADDR 10
#define PAYMENT_INDEX_ADDR  10
#define PAYMENT_RECORD_ADDR 10
//#define METER_RECORD_LIST_ADDR  10

int current_flash_record = 0;

void cleanFlashRecord() {
  uint32_t i;
  Serial.println("-- Flash -- Clean config file.");
  for (i = SYSTEM_INIT_ADDR; i < PAYMENT_RECORD_ADDR; i++) {
    //Serial.println(i);
    storageWriteByte(i, 0x00);
  }
  Serial.println("-- Flash -- Clean config file success.");
}

void readInitCommand() {
  if (sd_status == SD_EXIST) {
    File comFile;

    //comFile = openSdFile("init.ini", FILE_READ);
    if (comFile || 1) {
      char ss;
      uint16_t lineLen;
      pinMode(11, INPUT);

      if (!SD.exists("/credit")) {
        SD.mkdir("credit");
      }
      if (!SD.exists("/record")) {
        SD.mkdir("record");
      }
      if (!SD.exists("/system")) {
        SD.mkdir("system");
      }
      if (!SD.exists("/charge")) {
        SD.mkdir("charge");
      }

      //ss = comFile.read();
      //comFile.close();
      //byte flash_flag = storageReadByte(SYSTEM_INIT_ADDR);
      //Serial.print("--Init-- init flag : ");
      //Serial.println(flash_flag, HEX);
      pinMode(33,OUTPUT);
      digitalWrite(33,HIGH);
      delay(10);
      Serial.print("Config Pin Set : ");
      Serial.println(digitalRead(11));
      if (/*(ss == '1') || (flash_flag != 0x11) ||*/ (digitalRead(11) == HIGH)) {
        //storageWriteByte(SYSTEM_INIT_ADDR, (byte)0x11);
        Serial.println("--Init Status-- Reload extenal new config file --");
        readMeterObjectSD();
        cleanFlashRecord();
        saveFlashRecord(1);
        saveFlashTariff();
        /*
        if (ss=='1'){
          SD.remove("init.ini");
          delay(500);
          comFile = openSdFile("init.ini", FILE_WRITE);
          if (comFile) {
            comFile.print("0\n");
            comFile.flush();
            comFile.close();
          }
        }
        */
        return;
      }
    }
  }
  Serial.println("--Init Status-- Load internal config file --");
  // find a latest time

  int n_t = 0;
  uint32_t pre = 0, cur = 0,i;
  char *ps;
  //read meter box number
  meter_box_number = storageReadByte(METER_NUMBER_ADDR);

  if (meter_box_number == 0) {
    storageWriteByte(SYSTEM_INIT_ADDR, 0x01);
    Serial.println("-- Meter Init -- Read Flash Error ! -- No meter record, Please setup config.ini and init.ini Or try to reboot");
    systemErrorHandler(2);
  }

  user_number = meter_box_number * METER_PER_BOX;

  for (i = 0; i < SN_LENGTH; i++) {
    mg_sn[i] = (char)(0xff & storageReadByte(MINI_GRID_SN_ADDR + i));
  }
  mg_sn[i] = 0;

  vendor_number = (uint8_t)storageReadByte(VENDOR_NUMBER_ADDR);

  ps = (char *)vendor_credit_list;
  for (i = 0; i < sizeof(long) * VENDOR_NUMBER; i++)
    ps[i] = storageReadByte(VENDOR_CREDIT_ADDR + i);

  ip_self[0] = storageReadByte(STATIC_IP_ADDR);
  ip_self[1] = storageReadByte(STATIC_IP_ADDR + 1);
  ip_self[2] = storageReadByte(STATIC_IP_ADDR + 2);
  ip_self[3] = storageReadByte(STATIC_IP_ADDR + 3);

  ip_cc[0] = storageReadByte(CHARGE_CONTROLLER_IP_ADDR);
  ip_cc[1] = storageReadByte(CHARGE_CONTROLLER_IP_ADDR + 1);
  ip_cc[2] = storageReadByte(CHARGE_CONTROLLER_IP_ADDR + 2);
  ip_cc[3] = storageReadByte(CHARGE_CONTROLLER_IP_ADDR + 3);

  readFlashBattVolConfig();

  ps = (char *)&cur;
  for (i = 0; i < FLASH_MAX_RECORD; i++) {
    uint16_t start_addr = TIME_RECORD_LIST_ADDR + i * sizeof(uint32_t);
    ps[0] = storageReadByte(start_addr);
    ps[1] = storageReadByte(start_addr + 1);
    ps[2] = storageReadByte(start_addr + 2);
    ps[3] = storageReadByte(start_addr + 3);
    Serial.print("Time Record : "); Serial.println(cur);
    if ((cur > 900000000L) && (cur > pre)) {
      pre = cur;
      n_t = i;
    }
  }
  // read price tariff
  readFlashTariff();

  meter_box_number = 2;
  initMeterDataStruct();
  Serial.print("Record Area Addr : "); Serial.println(METER_RECORD_LIST_ADDR);
  uint16_t read_start = (n_t * sizeof(Meter_Box_Type) * meter_box_number);
  read_start += METER_RECORD_LIST_ADDR + 3;
  read_start &= 0xfffc;
  uint16_t read_end = read_start + sizeof(Meter_Box_Type) * meter_box_number;
  Serial.print("Record Addr : "); Serial.println(n_t);
  Serial.print("Read Objet from Addr : "); Serial.println(read_start);
  Serial.print("read Objet  to  Addr : "); Serial.println(read_end);
  ps = (char *)meter_box_list;
  for (i = read_start; i < read_end; i++) {
    *ps = storageReadByte(i);
    ps++;
  }

  for (int box = 0; box < meter_box_number; box++) {
    for (int meter = 0; meter < METER_PER_BOX; meter++) {
      meter_box_list[box].meter_list[meter].exc = 1;
    }
  }

  /*
    meter_box_list[1].meter_list[9].kwh = 0;
    meter_box_list[1].meter_list[9].credit = 0;
    meter_box_list[1].meter_list[9].kwh_bak = 0;
    meter_box_list[1].meter_list[9].flag = METER_STATUS_SWITCH_OFF;
    meter_box_list[1].meter_list[9].protect_time = 0;
  */
  //meter_box_list[0].meter_list[0].credit = 200;
  //meter_box_list[0].meter_list[0].flag = METER_STATUS_POWER_ON;
}

void printMeterObjects() {
  int i, j;
  char output[256];
  Serial.println("------------------------------ Info Start--------------------------");
  Serial.print("SN : "); Serial.println((char *)mg_sn);
  Serial.print("SN(HEX) : ");
  for (i = 0; i < SN_LENGTH; i++) {
    Serial.print(mg_sn[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  sprintf(output, "IP address :  %hhu.%hhu.%hhu.%hhu", ip_self[0], ip_self[1], ip_self[2], ip_self[3]);
  //Serial.println(ip_self[0],DEC);
  //Serial.println(ip_self[1],DEC);
  //Serial.println(ip_self[2],DEC);
  //Serial.println(ip_self[3],DEC);
  Serial.println(output);
  sprintf(output, "CC IP address : %hhu.%hhu.%hhu.%hhu", ip_cc[0], ip_cc[1], ip_cc[2], ip_cc[3]);
  Serial.println(output);
  sprintf(output, "Number of Meters : %d", meter_box_number);
  Serial.println(output);

  Serial.println("Price Table : ");
  Serial.print(price_tariff.price1_start_time);
  Serial.print("\t");
  Serial.println(price_tariff.price1);
  Serial.print(price_tariff.price2_start_time);
  Serial.print("\t");
  Serial.println(price_tariff.price2);
  Serial.print("Number of Vendors : "); Serial.println(vendor_number);
  Serial.println("MeterOjbect Start...\n");

  for (i = 0; i < meter_box_number; i++) {
    for (j = 0; j < METER_PER_BOX; j++) {
      Serial.print("Box: "); Serial.print(meter_box_list[i].id);
      Serial.print(", Meter: "); Serial.print(j + 1);
      Serial.print(" -- Used Power: "); Serial.print(meter_box_list[i].meter_list[j].kwh);
      Serial.print(" -- Amp: "); Serial.print(meter_box_list[i].meter_list[j].amp);
      Serial.print(" -- Watt: "); Serial.print(meter_box_list[i].meter_list[j].watt);
      Serial.print(", Credit: "); Serial.println(meter_box_list[i].meter_list[j].credit);
      //sprintf(output,"Box: %d, Meter: %d --- Used Power: %lu , Credit: %lu",meter_box_list[i].id,j+1,
      //    meter_box_list[i].meter_list[j].kwh,meter_box_list[i].meter_list[j].credit);
      //Serial.println(output);
    }
    Serial.println();
  }
  Serial.println("------------------------------ Info End--------------------------");
}

void saveFlashRecord(int sig) {
  int i;
  char *ps;
  Serial.println("-- Flash -- Save config file to flash......");

  if (sig == 1) {
    uint16_t temp_number = (uint16_t)storageReadByte(METER_NUMBER_ADDR);
    if (temp_number != meter_box_number) {
      storageWriteByte(METER_NUMBER_ADDR, meter_box_number);
    }

    storageWriteByte(STATIC_IP_ADDR, (char)ip_self[0]);
    storageWriteByte(STATIC_IP_ADDR+1, (char)ip_self[1]);
    storageWriteByte(STATIC_IP_ADDR+2, (char)ip_self[2]);
    storageWriteByte(STATIC_IP_ADDR+3, (char)ip_self[3]);
    storageWriteByte(CHARGE_CONTROLLER_IP_ADDR, (char)ip_cc[0]);
    storageWriteByte(CHARGE_CONTROLLER_IP_ADDR+1, (char)ip_cc[1]);
    storageWriteByte(CHARGE_CONTROLLER_IP_ADDR+2, (char)ip_cc[2]);
    storageWriteByte(CHARGE_CONTROLLER_IP_ADDR+3, (char)ip_cc[3]);
    //storageWriteBytes(CHARGE_CONTROLLER_IP_ADDR, (byte *)ip_cc, 4);
    saveFlashBattVolConfig();

    for (i = 0; i < SN_LENGTH; i++)
      storageWriteByte(MINI_GRID_SN_ADDR + i, (char)mg_sn[i]);
    //Serial.print("-- Flash Save -- Read SN -- ");
    //Serial.println((char *)mg_sn);
    //storageWriteBytes(MINI_GRID_SN_ADDR, (byte *)mg_sn, SN_LENGTH);

    storageWriteByte(VENDOR_NUMBER_ADDR, (char)vendor_number);
    ps = (char *)vendor_credit_list;
    for (i = 0; i < sizeof(long) * VENDOR_NUMBER; i++)
      ps[i] = storageWriteByte(VENDOR_CREDIT_ADDR + i,ps[i]);
  }

  ps = (char *)&systime;;
  uint16_t start_addr = TIME_RECORD_LIST_ADDR + current_flash_record * sizeof(uint32_t);
  storageWriteByte(start_addr, ps[0]);
  storageWriteByte(start_addr + 1, ps[1]);
  storageWriteByte(start_addr + 2, ps[2]);
  storageWriteByte(start_addr + 3, ps[3]);
  //storageWriteBytes(start_addr, (byte *)ps, sizeof(systime));

  uint16_t write_start = current_flash_record * sizeof(Meter_Box_Type) * meter_box_number+3;
  write_start += METER_RECORD_LIST_ADDR;
  write_start &= 0xfffc;
  uint16_t write_end = write_start + sizeof(Meter_Box_Type) * meter_box_number;
  Serial.print("Save Objet at Record : "); Serial.println(current_flash_record);
  Serial.print("Save Objet Head Addr : "); Serial.println(METER_RECORD_LIST_ADDR);
  Serial.print("Save Objet at Offset : "); Serial.println((current_flash_record * sizeof(Meter_Box_Type) * meter_box_number+3)&0xfffc);
  Serial.print("Save Objet from Addr : "); Serial.println(write_start);
  Serial.print("Save Objet  to  Addr : "); Serial.println(write_end);
  ps = (char *)meter_box_list;

  storageWriteBytes(write_start, (byte *)ps, sizeof(Meter_Box_Type) * meter_box_number);
  current_flash_record++;
  if (current_flash_record >= FLASH_MAX_RECORD) current_flash_record = 0;
  Serial.println("-- Flash -- Finish Save config file to flash");
}

void saveFlashTariff() {
  byte *ps;
  ps = (byte *)&price_tariff;
  uint32_t head = (uint32_t)TARIFF_ADDR;
  for (int i=0;i<sizeof(price_tariff);i++){
    storageWriteByte(head+i,ps[i]);
  }

  ps = (byte *)&price_tariff;
  head = (uint32_t)NEW_TARIFF_ADDR;
  for (int i=0;i<sizeof(new_price_tariff);i++){
    storageWriteByte(head+i,ps[i]);
  }
}

void readFlashTariff() {
  char *ps;
  uint16_t read_start = TARIFF_ADDR;
  uint16_t read_end = read_start + sizeof(price_tariff);
  ps = (char *)&price_tariff;
  for (uint16_t i = read_start; i < read_end; i++) {
    ps[0] = storageReadByte(i);
    ps++;
  }
  read_start = NEW_TARIFF_ADDR;
  read_end = read_start + sizeof(new_price_tariff);
  ps = (char *)&new_price_tariff;
  for (uint16_t i = read_start; i < read_end; i++) {
    ps[0] = storageReadByte(i);
    ps++;
  }
}

void readFlashBattVolConfig(){
  char *ps;
  ps = (char *)(&batt_protect_vol);
  ps[0] = storageReadByte(BATT_PROTECT_VOLTAGE_ADDR);
  ps[1] = storageReadByte(BATT_PROTECT_VOLTAGE_ADDR+1);
  ps = (char *)(&batt_recover_vol);
  ps[0] = storageReadByte(BATT_RECOVER_VOLTAGE_ADDR);
  ps[1] = storageReadByte(BATT_RECOVER_VOLTAGE_ADDR+1);
}

void saveFlashBattVolConfig(){
  char *ps;
  ps = (char *)(&batt_protect_vol);
  storageWriteByte(BATT_PROTECT_VOLTAGE_ADDR,ps[0]);
  storageWriteByte(BATT_PROTECT_VOLTAGE_ADDR+1,ps[1]);
  ps = (char *)(&batt_recover_vol);
  storageWriteByte(BATT_RECOVER_VOLTAGE_ADDR,ps[0]);
  storageWriteByte(BATT_RECOVER_VOLTAGE_ADDR+1,ps[1]);
}

/*  *******************  Meter Init File *********************** */
/*       SN 0123456789ABCDEF                                     */
/*       IP 10.0.0.2                                             */
/*       CCIP 10.0.0.9
/*       PTV 210 REV 225                                         */
/*       Tariff  0.58 | 1427865468 1.20                */
/*       VendorNumber 2                                          */
/*       VendorCredit 222 333                                    */
/*       MeterNumber n                                           */
/*       SN 10.0.0.21             *SN is number                  */
/*       UserCredit 0 0 0 0 0 0 0 0 0 0                          */
/*                                                               */

#pragma GCC push_options
#pragma GCC optimize ("O0")


void readMeterObjectSD() {
  char logs[200];
  byte tempip[4];
  if (sd_status == SD_EXIST) {
    int create_new_files = -1;
    File initFile;
    initFile = openSdFile("config2.ini", FILE_READ);
    if (initFile) {
      char line[201];
      uint16_t lineLen;

      //read SN
      lineLen = initFile.readBytesUntil('\n', line, 100);
      line[lineLen] = 0;
      if (line[0] == 'S' && line[1] == 'N' && lineLen >= 19) {
        char *sts = strstr(line, " ") + 1;
        int i;
        for (i = 0; i < SN_LENGTH; i++) {
          mg_sn[i] = sts[i];
        }
        mg_sn[i] = 0;
        Serial.print("SN: "); Serial.println((char *)mg_sn);
      }
      else {
        Serial.println("-- Meter Init -- Read config.ini Error ! -- SN Format Error !");
        systemErrorHandler(0);
      }

      Serial.print("-- Meter Init -- Read SN -- ");
      Serial.println((char *)mg_sn);




      //read IP
      lineLen = initFile.readBytesUntil('\n', line, 100);
      line[lineLen] = 0;

      if (!sscanf(line, "IP %hhu.%hhu.%hhu.%hhu", &tempip[0], &tempip[1],&tempip[2], &tempip[3])) {
        Serial.println("-- Meter Init -- Read config.ini Error ! -- MiniGrid IP Format Error ");
        systemErrorHandler(0);
      }

      //Serial.println(line);
      //sprintf(logs, "-- Meter Init -- Read IP -- %hhu.%hhu.%hhu.%hhu", tempip[0], tempip[1], tempip[2], tempip[3]);
      //Serial.println(logs);

      //read charge controller IP
      lineLen = initFile.readBytesUntil('\n', line, 100);
      line[lineLen] = 0;

      if (!sscanf(line, "CCIP %hhu.%hhu.%hhu.%hhu", &ip_cc[0], &ip_cc[1], &ip_cc[2], &ip_cc[3]))
      {
        Serial.println("-- Meter Init -- Read config.ini Error ! -- MiniGrid Charge Controller Format Error ");
        systemErrorHandler(0);
      }

      ip_self[0] = tempip[0];
      ip_self[1] = tempip[1];
      ip_self[2] = tempip[2];
      ip_self[3] = tempip[3];
      sprintf(logs, "-- Meter Init -- Read IP -- %hhu.%hhu.%hhu.%hhu", ip_self[0], ip_self[1], ip_self[2], ip_self[3]);
      Serial.println(logs);

      sprintf(logs, "-- Meter Init -- Read CCIP -- %hhu.%hhu.%hhu.%hhu", ip_cc[0], ip_cc[1], ip_cc[2], ip_cc[3]);
      Serial.println(logs);

      //read battery voltage config

      lineLen = initFile.readBytesUntil('\n', line, 150);
      line[lineLen] = 0;

      if (!sscanf(line, "PTV %u REV %u", &batt_protect_vol, &batt_recover_vol)) {
        Serial.println("-- Meter Init -- Read config.ini Error ! -- MiniGrid battery config Error ");
        systemErrorHandler(0);
      }
      else{
        sprintf(logs, "-- Meter Init -- Read Battery config -- protect vol: %d --recover vol: %d", batt_protect_vol,batt_recover_vol);
        Serial.println(logs);
      }

      //read traff mode
      lineLen = initFile.readBytesUntil('\n', line, 100);
      line[lineLen] = 0;

      char *pstr, *numstart;
      numstart = strstr(line, " ");
      numstart += 1;
      price_tariff.price1_start_time = strtol(numstart, &pstr, 10);
      //Serial.println(price_tariff.price1_start_time);
      numstart = strstr(numstart, " ") + 1;
      //Serial.println(numstart);
      price_tariff.price1 = strtol(numstart, &pstr, 10);
      //Serial.println(price_tariff.price1);
      numstart = strstr(numstart, " ") + 1;
      //Serial.println(numstart);
      price_tariff.price2_start_time = strtol(numstart, &pstr, 10);
      //Serial.println(price_tariff.price2_start_time);
      numstart = strstr(numstart, " ") + 1;
      //Serial.println(numstart);
      price_tariff.price2 = strtol(numstart, &pstr, 10);
      //Serial.println(price_tariff.price2);
      price_tariff.generate_time = 1;
      price_tariff.init_time = 1;

      new_price_tariff.price1_start_time = 0;
      new_price_tariff.price2_start_time = 0;
      new_price_tariff.price1 = 0;
      new_price_tariff.price2 = 0;
      new_price_tariff.generate_time = 0;
      new_price_tariff.init_time = 0;

      Serial.print("-- Meter Init -- Tariff Table -- ");
      Serial.println(line);

      //read vendor and vendor credit
      lineLen = initFile.readBytesUntil('\n', line, 100);
      line[lineLen] = 0;
      if (!sscanf(line, "VendorNumber %d", &vendor_number)) {
        Serial.println("-- Meter Init -- Read config.ini Error ! -- Vendor Number Format Error ");
        systemErrorHandler(0);
      }

      lineLen = initFile.readBytesUntil('\n', line, 100);
      line[lineLen] = 0;
      if (!sscanf(line, "VendorCredit %ld %ld", &vendor_credit_list[0], &vendor_credit_list[1])) {
        Serial.println("-- Meter Init -- Read config.ini Error ! -- Vendor Credit Format Error ");
        systemErrorHandler(0);
      }
      Serial.print("-- Meter Init -- Vendor Number -- ");
      Serial.print(vendor_number);
      Serial.print(" -- Vendor Credit -- ");
      Serial.print(vendor_credit_list[0]);
      Serial.print(" -- ");
      Serial.println(vendor_credit_list[1]);

      //read meter quantity and credit
      lineLen = initFile.readBytesUntil('\n', line, 100);
      line[lineLen] = 0;
      numstart = strstr(line, " ");
      numstart += 1;
      meter_box_number = strtol(numstart, &pstr, 10);
      /*
        if (!sscanf(line, "MeterNumber %d", &meter_box_number)) {
        Serial.println("-- Meter Init -- Read config.ini Error ! -- Meter Number Format Error ");
        systemErrorHandler(0);
        }
      */
      Serial.print("-- Meter Init -- Meter Number -- ");
      Serial.println(meter_box_number);

      user_number = meter_box_number * METER_PER_BOX;

      if (meter_box_number <= 0) {
        Serial.println("-- Meter Init -- Read config.ini Error ! -- Meter Number is 0 ");
        systemErrorHandler(0);
      }

      initMeterDataStruct();

      for (int i = 0; i < meter_box_number; i++) {
        lineLen = initFile.readBytesUntil('\n', line, 100);
        line[lineLen] = 0;
        Serial.print(" -- Meter Init -- Read Meter From config.ini -- ");
        Serial.println(line);
        if (!sscanf(line, "%d %hhu.%hhu.%hhu.%hhu %u", &meter_box_list[i].id, &meter_box_list[i].ip[0], &meter_box_list[i].ip[1],
                    &meter_box_list[i].ip[2], &meter_box_list[i].ip[3], &meter_box_list[i].port)) {
          Serial.println("-- Meter Init -- Read config.ini Error ! -- Meter Format Error ");
          systemErrorHandler(0);
        }
        char temps[200];
        sprintf(temps, "---    %d %hhu.%hhu.%hhu.%hhu:%u", meter_box_list[i].id, meter_box_list[i].ip[0], meter_box_list[i].ip[1],
                meter_box_list[i].ip[2], meter_box_list[i].ip[3], meter_box_list[i].port);
        Serial.println(temps);
      }

      // Init user credit
      lineLen = initFile.readBytesUntil('\n', line, 200);
      line[lineLen] = 0;
      char *nn = line;
      for (int box = 0; box < meter_box_number; box++) {
        for (int meter = 0; meter < METER_PER_BOX; meter++) {
          nn = strstr(nn, " ") + 1;
          if (!sscanf(nn, "%ld", &meter_box_list[box].meter_list[meter].credit)) {
            Serial.print("-- Meter Init -- Read config.ini Error ! -- User Credit Format Error -- ");
            Serial.println(box * 10 + meter + 1);
            systemErrorHandler(0);
          }
        }
      }

      // Init Object Values

      Serial.println("-- Read Object from SD card success !");
/*
      if (!SD.exists("/credit")) {
        SD.mkdir("credit");
      }
      if (!SD.exists("/record")) {
        SD.mkdir("record");
      }
      if (!SD.exists("/system")) {
        SD.mkdir("system");
      }
      if (!SD.exists("/charge")) {
        SD.mkdir("charge");
      }
*/
      return;
      // update code

      // read history
      char line2[1000];
      char file_name[11];
      File timeFile = openSdFile("time.txt", FILE_READ);
      if (timeFile) {
        uint32_t len = timeFile.size();
        if (len >= 11) timeFile.seek(len - 9);
        timeFile.read(file_name, 8);
        file_name[8] = 0;
        Serial.print(" -- Meter Init -- Read time.txt -- Get Previous File Name : ");
        Serial.print(file_name);
        Serial.println(".log");
        create_new_files = -1;
      }
      else {
        Serial.println("-- Meter Init -- Read time.txt Error ! -- Can't open TIMERECORD.TXT ");
        Serial.println("-- Meter Init -- Read time.txt Error ! -- Will Create a New TIMERECORD.TXT ");
        create_new_files = 1;
        File timef = openSdFile("time.txt", FILE_WRITE);
        timef.close();
        //systemErrorHandler(0);
      }
      timeFile.close();

      if (create_new_files == -1) {
        char local_file_name[25];

        sprintf(local_file_name, "/credit/%s.log", file_name);
        File creditFile = openSdFile(local_file_name, FILE_READ);
        if (creditFile) {
          readLastLine(creditFile, line2);
          Serial.println("-- Meter Init -- Read Credit Log File Data -- ");
          Serial.println(line2);
          char *head;
          uint16_t m_number;
          head = line2;
          head = strstr(line2, "*");
          if (!sscanf(head + 1, "%d", &m_number)) {
            Serial.println("-- Meter Init -- Read Credit Log File Error ! -- Can't read meter number ");
            Serial.println("All the credits will be setted to Zero !");
            m_number = 0;
            //systemErrorHandler(0);
          }
          int j, k, h;
          for (j = 0; j < m_number; j++) {
            head = strstr(head, "|");
            int m_id;
            sscanf(head + 1, "%d", &m_id);
            Serial.print("-- Meter Init -- Read Credit Log -- Read meter ID : ");
            Serial.println(m_id, DEC);

            // search meter box
            for (k = 0; k < meter_box_number; k++) {
              if (meter_box_list[k].id == m_id) {
                break;
              }
            }
            // if no resault
            if (k >= meter_box_number) continue;

            head = strstr(head, ": ");
            sscanf(head + 2, "%lu , %lu , %lu , %lu , %lu , %lu , %lu , %lu , %lu , %lu",
                   &meter_box_list[j].meter_list[0].credit,
                   &meter_box_list[j].meter_list[1].credit,
                   &meter_box_list[j].meter_list[2].credit,
                   &meter_box_list[j].meter_list[3].credit,
                   &meter_box_list[j].meter_list[4].credit,
                   &meter_box_list[j].meter_list[5].credit,
                   &meter_box_list[j].meter_list[6].credit,
                   &meter_box_list[j].meter_list[7].credit,
                   &meter_box_list[j].meter_list[8].credit,
                   &meter_box_list[j].meter_list[9].credit
                  );
            char temps[500];
            sprintf(temps, "%lu\n%lu\n%lu\n%lu\n%lu\n%lu\n%lu\n%lu\n%lu\n%lu\n",
                    meter_box_list[j].meter_list[0].credit,
                    meter_box_list[j].meter_list[1].credit,
                    meter_box_list[j].meter_list[2].credit,
                    meter_box_list[j].meter_list[3].credit,
                    meter_box_list[j].meter_list[4].credit,
                    meter_box_list[j].meter_list[5].credit,
                    meter_box_list[j].meter_list[6].credit,
                    meter_box_list[j].meter_list[7].credit,
                    meter_box_list[j].meter_list[8].credit,
                    meter_box_list[j].meter_list[9].credit
                   );
            Serial.println(temps);
          }
          creditFile.close();
        }
        else {
          Serial.println("-- Meter Init -- Read Credit Log File Error ! -- Can't open credit log ");
          Serial.println("All the credits will be setted to Zero !");
        }

        // read record file
        sprintf(local_file_name, "/record/%s.log", file_name);
        File recordFile = openSdFile(local_file_name, FILE_READ);
        if (recordFile) {
          readLastLine(recordFile, line2);
          Serial.println("-- Meter Init -- Read Record Log File Data -- ");
          Serial.println(line2);
          char *head;
          uint16_t m_number;
          head = line2;
          head = strstr(head, " *");
          if (!sscanf(head, " *%d", &m_number)) {
            Serial.println("-- Meter Init -- Read Record Log File Error ! -- Can't read meter number ");
            Serial.println("All the credits will be setted to Zero !");
            m_number = 0;
            //systemErrorHandler(0);
          }
          int j, k, h;
          for (j = 0; j < m_number; j++) {
            head = strstr(head, " |");
            int m_id;
            sscanf(head, " |%d", &m_id);
            Serial.print("-- Meter Init -- Read Record Log -- Read meter ID : ");
            Serial.println(m_id, DEC);
            // search meter box
            for (k = 0; k < meter_box_number; k++) {
              if (meter_box_list[k].id == m_id) {
                break;
              }
            }
            // if no resault
            if (k >= meter_box_number) continue;
            double t_credit;
            for (h = 0; h < METER_PER_BOX; h++) {
              if (h == 0) {
                head = strstr(head, ": ");
                if (!head) continue;
                head += 2;
              }
              else {
                head = strstr(head, ", ");
                if (!head) continue;
                head += 2;
              }
              sscanf(head, "%lu %lu %lu %lu %hhu",
                     &meter_box_list[k].meter_list[h].amp,
                     &meter_box_list[k].meter_list[h].watt,
                     &meter_box_list[k].meter_list[h].kwh,
                     &t_credit,
                     &meter_box_list[k].meter_list[h].flag);
              char temps[200];
              sprintf(temps, "--  %lu %lu %lu %lu %hhu",
                      meter_box_list[k].meter_list[h].amp,
                      meter_box_list[k].meter_list[h].watt,
                      meter_box_list[k].meter_list[h].kwh,
                      t_credit,
                      meter_box_list[k].meter_list[h].flag);
              Serial.println(temps);
            }
            meter_box_list[j].updated = METER_DATA_UPDATED;
          }
          recordFile.close();
        }
        else {
          Serial.println("-- Meter Init -- Read Record Log File Error ! -- Can't open record log ");
          Serial.println("All the record will be setted to Zero !");
        }
      }
      else {
        // if create new files
        Serial.println("-- Meter Init -- No any record -- Generate New credit files and record files ");
      }
      initFile.close();
    }
    else {
      Serial.println("-- Meter Init -- config.ini Can't open ");
      systemErrorHandler(0);
    }
  }
}

#pragma GCC pop_options

uint16_t readLastLine(File file, char *data) {
  uint32_t file_size = file.size();
  uint16_t len = 0;
  while (1) {
    len = file.readBytesUntil('\n', data, 1000);
    uint32_t follow = file.size() - file.position();
    if (follow == 0) follow = 1;
    data[len] = 0;
    float percent = (float)len / (float)follow;
    if (percent < 0.1) {
      file.seek(file.position() + len * 10);
    }
    else if (percent < 0.2) {
      file.seek(file.position() + len * 4 );
    }
    else if (percent > 2.0) {
      break;
    }
  }
  return len;
}

void initMeterDataStruct() {
  memset(dynamic_memery, 0, sizeof(dynamic_memery));

  meter_box_list = (Meter_Box_Type *)dynamic_memery;
  protect_time_list = (uint32_t *)(dynamic_memery + sizeof(Meter_Box_Type) * meter_box_number);
  temp_kwh_list = (uint32_t *)(dynamic_memery + sizeof(Meter_Box_Type) * meter_box_number + sizeof(uint32_t) * meter_box_number * METER_PER_BOX);
  /*
    meter_box_list = new Meter_Box_Type[meter_box_number];

    protect_time_list = new uint32_t*[meter_box_number];
    temp_kwh_list = new uint32_t*[meter_box_number];

    uint32_t *temp = new uint32_t[meter_box_number * METER_PER_BOX * 2];
    for (int cc = 0; cc < meter_box_number; cc++) {
    protect_time_list[cc] = temp + METER_PER_BOX * cc;
    temp_kwh_list[cc] = temp + meter_box_number * METER_PER_BOX + METER_PER_BOX * cc;
    }
  */
  for (int j = 0; j < meter_box_number; j++) {
    meter_box_list[j].port = 0;
    meter_box_list[j].flag = METER_BOX_OFFLINE;
    meter_box_list[j].updated = METER_DATA_INIT;
    for (int k = 0; k < METER_PER_BOX; k++) {
      meter_box_list[j].meter_list[k].amp = 0;
      meter_box_list[j].meter_list[k].watt = 0;
      meter_box_list[j].meter_list[k].kwh = 0;
      meter_box_list[j].meter_list[k].credit = 0;
      meter_box_list[j].meter_list[k].exc = 1;
      meter_box_list[j].meter_list[k].flag = METER_STATUS_SWITCH_OFF;
    }
  }
}

/************************************************* Meter Task Group ********************************************/

#define METER_TASK_IDEL             0
#define METER_TASK_CONNECT_TO_BOX   1
#define METER_TASK_CONNECT_CHECK    2
#define METER_TASK_NORMAL_SEND      3
#define METER_TASK_NORMAL_REPLY     4
#define METER_TASK_EXC_COMMAND      5
#define METER_TASK_EXC_REPLY        6
#define METER_TASK_SET_COIL         7
#define METER_TASK_DISCONNECT       8

typedef struct {
  uint16_t box_id;
  uint16_t box_rank;
  uint16_t meter;
  uint8_t coil;
} Mission_Type;
#define MM_TYPE_ADD_CREDIT  1
#define MM_TYPE_CONTROL     2

EthernetClient meterConn;

#define MAX_MISSION_NUMBER  5

Mission_Type meter_mission_list[5];
int meter_mission_number = 0;
int current_mission = -1;

uint8_t connection_status = 0;
#define CONN_DISCONNECT   1
#define CONN_INIT         2
#define CONN_CONNECTED    3

uint8_t meter_task_step = 0;

OS_TASK *meter_main_task = NULL , *meter_maintenance_task = NULL;
int meter_box_rolling = 0, meter_rolling = 0;
int connected_box_id = -1;
int connected_box_rank = -1;
uint8_t meter_receive_data[100];
int meter_should_receive = 0;
int meter_receive = 0;
uint8_t connect_retry_time = 0;
uint8_t item_rolling = 0;

#define MAX_RETRY    6
#define NEXT_COMMAND_INTERVAL     OS_ST_PER_100_MS


unsigned meterMainTask(int opt) {
  char logs[200];

  switch (meter_task_step) {
    case METER_TASK_IDEL : {
        if (charge_controller_status != CHARGE_CONTROLLOR_VOLTAGE_IDEL) {
          meter_task_step = METER_TASK_CONNECT_TO_BOX;
          //Serial.println("--MainTask-- INIT");
        }
        else {
          break;
        }
      }
    case METER_TASK_CONNECT_TO_BOX : {
        //Serial.println("\n\n***********************************************************\n\n");
        if (meter_mission_number > 0) {
          //command exist;
          current_mission = meter_mission_number - 1;
          //Serial.println("--MainTask-- Mission Exist");
          if (connection_status == CONN_CONNECTED && connected_box_rank == meter_mission_list[current_mission].box_rank) {
            // if already connected
            meter_task_step = METER_TASK_EXC_COMMAND;
            //Serial.println("--MainTask-- Mission Box Already Connected");
          }
          else {
            // if not connected to right box
            meter_task_step = METER_TASK_CONNECT_CHECK;
            //Serial.println("--MainTask-- Connecting to Mission meter");
            connectToMeterRank(meter_mission_list[current_mission].box_rank);
          }
          break;
        }
        else {
          // no command
          //Serial.println("--MainTask--No Mission, Connecting to meter");
          if (connection_status == CONN_CONNECTED && connected_box_id == meter_box_list[meter_box_rolling].id) {
            // if already connected
            meter_task_step = METER_TASK_NORMAL_SEND;
            //Serial.println("--MainTask-- Destnation Meter Already Connected");
          }
          else {
            meter_task_step = METER_TASK_CONNECT_CHECK;
            connectToMeterRank(meter_box_rolling);
          }
          break;
        }
      }
    case METER_TASK_CONNECT_CHECK : {
        if (current_mission > -1 ) {
          // mission exist
          if (connection_status == CONN_CONNECTED) {
            // connect success
            Serial.println("--MainTask-- Mission Connect to meter Success");
            meter_task_step = METER_TASK_EXC_COMMAND;
            meter_box_list[meter_box_rolling].flag = METER_BOX_ONLINE;
            //connect_retry_time = 0;
            selfNextDutyDelay(OS_ST_PER_100_MS);
            break;
          }
          else {
            Serial.println("--MainTask-- Mission Connect to meter Failed");
            // connect fail
            meter_box_list[meter_box_rolling].error = 1;
            connect_retry_time++;
            connectionStop();
            meter_box_list[meter_box_rolling].flag = METER_BOX_OFFLINE;
            meter_task_step = METER_TASK_CONNECT_TO_BOX;
            selfNextDutyDelay(OS_ST_PER_100_MS * 3);
            if (connect_retry_time > MAX_RETRY) {
              meter_box_list[connected_box_rank].meter_list[meter_mission_list[current_mission].meter].exc = 1;
              meter_mission_number--; // fail del the mission;
              connect_retry_time = 0;
              current_mission = -1;
              systemMaintain(0);
            }
            break;
            //meter_task_step = METER_TASK_NORMAL_SEND;
          }
        }
        else {
          // no mission
          if (connection_status == CONN_CONNECTED) {
            // connect success
            //Serial.println("--MainTask-- Connection Established");
            meter_box_list[meter_box_rolling].flag = METER_BOX_ONLINE;
            meter_task_step = METER_TASK_NORMAL_SEND;
            meter_box_list[meter_box_rolling].error = 0;
            //connect_retry_time = 0;
            //break;
          }
          else {
            //Serial.println("--MainTask-- Connect to meter failed");
            // connect fail
            connect_retry_time++;
            connectionStop();
            meter_task_step = METER_TASK_CONNECT_TO_BOX;
            meter_box_list[meter_box_rolling].flag = METER_BOX_OFFLINE;
            selfNextDutyDelay(OS_ST_PER_100_MS);
            if (connect_retry_time > MAX_RETRY) {
              Serial.print("--MainTask-- Connect to meter failed, box : "); Serial.println(meter_box_rolling);
              connect_retry_time = 0;
              //if ((++meter_box_list[meter_box_rolling].error) > 10) meter_box_list[meter_box_rolling].error = 10;
              meter_box_list[meter_box_rolling].error = 1;
              // empty the meter status.
              if ((++meter_box_rolling) >= meter_box_number) {
                meter_box_rolling = 0; //shift to next box;
              }
              item_rolling = 0;
              meter_rolling = 0;
              systemMaintain(0);
            }
            break;
            //meter_task_step = METER_TASK_NORMAL_SEND;
          }
        }
      }
    case METER_TASK_NORMAL_SEND : {
        //Serial.println(" ------------------------------------------------------ ");
        //Serial.print("--MainTask-- Send query request to meter : ");
        //Serial.print(meter_box_rolling);
        //Serial.print(" - ");
        //Serial.println(meter_rolling);
        int m_flag = 0;
        uint32_t next_duty_adjust = 0;
        m_flag = -1;
        if (item_rolling == 3) {
          // read amp
          //Serial.println("--MainTask-- Read AMP");
          m_flag = readAmp(meter_rolling);
        }
        else if (item_rolling == 0) {
          // read watt
          //Serial.println("--MainTask-- Read Watt");
          m_flag = readWatt(meter_rolling);
        }
        else if (item_rolling == 1) {
          // read kwh
          //Serial.println("--MainTask-- Read kwh");
          m_flag = readKwh(meter_rolling);
        }
        else if (item_rolling == 2) {
          //Serial.println("--MainTask-- Check or Control Coil");
          if (meter_box_list[meter_box_rolling].meter_list[meter_rolling].exc == 1) {
            // if coil need to be changed
            if (meter_box_list[meter_box_rolling].meter_list[meter_rolling].flag == METER_STATUS_POWER_ON) {
              m_flag = switchCoil(meter_box_rolling, meter_rolling, 1);
              sprintf(logs, "%ld : METER %d in BOX %d be switched on ", systime, meter_rolling + 1, connected_box_id);
              saveSystemRecord(logs);
            }

            else if (meter_box_list[meter_box_rolling].meter_list[meter_rolling].flag == METER_STATUS_NO_CREDIT_OFF ||
                     meter_box_list[meter_box_rolling].meter_list[meter_rolling].flag == METER_STATUS_SWITCH_OFF ||
                     meter_box_list[meter_box_rolling].meter_list[meter_rolling].flag == METER_STATUS_OVER_AMP_OFF
                    ) {
              //Serial.println("--MainTask-- Trun Coil Off");
              sprintf(logs, "%ld : METER %d in BOX %d be switched off ", systime, meter_rolling + 1, connected_box_id);
              saveSystemRecord(logs);
              m_flag = switchCoil(meter_box_rolling, meter_rolling, 0);
            }
          }
          else if ((systime - protect_time_list[meter_box_rolling * METER_PER_BOX + meter_rolling]) > OVER_CURRENT_PROTECT_TIME &&
                   meter_box_list[meter_box_rolling].meter_list[meter_rolling].flag == METER_STATUS_OVER_AMP_OFF
                  ) {
            // if over current pass the protect time
            meter_box_list[meter_box_rolling].meter_list[meter_rolling].flag == METER_STATUS_POWER_ON;
            protect_time_list[meter_box_rolling * METER_PER_BOX + meter_rolling] == 0;
            meter_box_list[meter_box_rolling].meter_list[meter_rolling].exc = 1;
            next_duty_adjust = OS_ST_PER_100_MS * 2;
            //Serial.println("--MainTask-- Trun Coil On");
            sprintf(logs, "%ld : Over Current Protection Ends ! METER %d in BOX %d be switched on ", systime, meter_rolling + 1, connected_box_id);
            saveSystemRecord(logs);
            m_flag = switchCoil(meter_box_rolling, meter_rolling, 1);
          }
          else {
            // if coil no need to be change
            item_rolling++;
            meter_receive = 0;
            break;
          }
        }
        meter_receive = 0;
        if (m_flag == 1) {
          // Send success
          meter_task_step = METER_TASK_NORMAL_REPLY;
          selfNextDutyDelay(OS_ST_PER_100_MS * 10 + next_duty_adjust);
          //Serial.println("--MainTask-- Send Command OK !");
        }
        else {
          // Send Fail
          connect_retry_time++;
          connectionStop();
          meter_task_step = METER_TASK_CONNECT_TO_BOX;
          selfNextDutyDelay(OS_ST_PER_100_MS * 5);
          if (connect_retry_time > MAX_RETRY) {
            Serial.println("--MainTask-- Send to meter failed");
            connect_retry_time = 0;
            //if ((++meter_box_list[meter_box_rolling].error) > 10) meter_box_list[meter_box_rolling].error = 10;
            meter_box_list[meter_box_rolling].error = 2;
            // empty the meter status.
            //meter_box_rolling++;
            //if (meter_box_rolling >= meter_box_number) {
            //  meter_box_rolling = 0; //shift to next box;
            //}
            item_rolling += 1;
            if (item_rolling > 3) {
              item_rolling = 0;
              meter_rolling += 1;
              if (meter_rolling >= METER_PER_BOX) {
                meter_rolling = 0;
                meter_box_rolling += 1;
                if (meter_box_rolling >= meter_box_number) {
                  meter_box_rolling = 0; //shift to next box;
                }
              }
            }
            Serial.print("--MainTask-- Error check item number: ");
            Serial.println(item_rolling);
          }
        }

        break;
      }
    case METER_TASK_NORMAL_REPLY : {
        //Serial.println("--MainTask-- Check meter reply");
        if (meter_receive >= meter_should_receive && meter_should_receive != 0) {
          //Serial.print("--MainTask-- Get meter reply : ");
          // receive success
          //for (int ii = 0; ii < meter_receive; ii++) {
          //  Serial.print(meter_receive_data[ii], HEX);
          //  Serial.print(" ");
          //}
          //Serial.println();
          if (item_rolling == 3) {
            // amp
            meter_box_list[meter_box_rolling].meter_list[meter_rolling].amp = stringfloat2longAmp1000(meter_receive_data + 9);
            if ((int)meter_box_list[meter_box_rolling].meter_list[meter_rolling].amp >= OVER_CURRENT) {
              meter_box_list[meter_box_rolling].meter_list[meter_rolling].flag = METER_STATUS_OVER_AMP_OFF;
              protect_time_list[meter_box_rolling * METER_PER_BOX + meter_rolling] = systime;
              meter_box_list[meter_box_rolling].meter_list[meter_rolling].exc = 1;
              char logs[90];
              sprintf(logs, "%ld : Over Current Detected ! METER %d in BOX %d will be switched off ", systime, meter_rolling + 1, connected_box_id);
              saveSystemRecord(logs);
              meter_task_step = METER_TASK_NORMAL_SEND;
              item_rolling = 2;
            }
            else {
              item_rolling = 0;
              meter_rolling++;
              // shift to next box
              if (meter_rolling >= METER_PER_BOX) {
                meter_rolling = 0;

                connectionStop();
                meter_task_step = METER_TASK_CONNECT_TO_BOX;
                // shift to next box
                meter_box_rolling++;
                if (meter_box_rolling >= meter_box_number) {
                  meter_box_rolling = 0;
                }
              }
            }

            selfNextDutyDelay(NEXT_COMMAND_INTERVAL);
          }
          else if (item_rolling == 0) {
            // watt
            meter_box_list[meter_box_rolling].meter_list[meter_rolling].watt = stringfloat2longAmp1000(meter_receive_data + 9);
            item_rolling++;
            meter_task_step = METER_TASK_NORMAL_SEND;
            selfNextDutyDelay(NEXT_COMMAND_INTERVAL);
          }
          else if (item_rolling == 1) {
            // kwh
            uint32_t kwh = stringfloat2longAmp1000(meter_receive_data + 9);
            long gap = kwh - meter_box_list[meter_box_rolling].meter_list[meter_rolling].kwh;
            long temp_credit = meter_box_list[meter_box_rolling].meter_list[meter_rolling].credit - (long)((gap * current_price + 50) / 100);
            if (temp_credit <= 0) taskNextDutyDelay(meter_maintenance_task, 0); // seems no credit
            //Serial.print("Read KWH :"); Serial.println(kwh);
            temp_kwh_list[meter_box_rolling * METER_PER_BOX + meter_rolling] = kwh;
            item_rolling++;
            meter_task_step = METER_TASK_NORMAL_SEND;
            selfNextDutyDelay(NEXT_COMMAND_INTERVAL);
          }
          else if (item_rolling == 2) {
            meter_box_list[meter_box_rolling].meter_list[meter_rolling].exc = 0;
            meter_task_step = METER_TASK_NORMAL_SEND;
            item_rolling = 3;
            if (meter_receive_data[10] == 0x00) {

              selfNextDutyDelay(NEXT_COMMAND_INTERVAL);
            }
            else {
              // switch on check !
              selfNextDutyDelay(OS_ST_PER_SECOND);
            }
          }
          meter_box_list[meter_box_rolling].error = 0;
          connect_retry_time = 0;
          meter_task_step = METER_TASK_NORMAL_SEND;
        }
        else {
          // receive error
          Serial.println("--MainTask-- No reply from meter");
          connect_retry_time++;
          meter_task_step = METER_TASK_NORMAL_SEND;
          selfNextDutyDelay(NEXT_COMMAND_INTERVAL + OS_ST_PER_100_MS * 2);
          if (connect_retry_time > 2) {
            connectionStop();
            meter_task_step = METER_TASK_CONNECT_TO_BOX;
          }
          else if (connect_retry_time > MAX_RETRY) {
            connect_retry_time = 0;
            meter_box_list[meter_box_rolling].error = 3;
            // empty the meter status.
            connectionStop();
            meter_task_step = METER_TASK_CONNECT_TO_BOX;
            item_rolling += 1;
            if (item_rolling > 3) {
              item_rolling = 0;
              meter_rolling += 1;
              if (meter_rolling >= METER_PER_BOX) {
                meter_rolling = 0;
                meter_box_rolling += 1;
                if (meter_box_rolling >= meter_box_number) {
                  meter_box_rolling = 0; //shift to next box;
                }
              }
            }

          }
        }

        // if mission exists
        if (meter_mission_number > 0) {
          Serial.println("--MainTask-- shift to mission mode");
          meter_task_step = METER_TASK_CONNECT_TO_BOX;
        }
        meter_receive = 0;
        meter_should_receive = 0;
        //selfNextDutyDelay(0);
        break;
      }

    case METER_TASK_EXC_COMMAND : {
        if (switchCoil(meter_mission_list[current_mission].box_rank, meter_mission_list[current_mission].meter, meter_mission_list[current_mission].coil)) {
          meter_task_step = METER_TASK_EXC_REPLY;
          meter_box_list[connected_box_rank].error = 0;
          selfNextDutyDelay(OS_ST_PER_100_MS * 5);
        }
        else {
          connect_retry_time++;
          connectionStop();
          meter_task_step = METER_TASK_CONNECT_TO_BOX;
          selfNextDutyDelay(OS_ST_PER_100_MS * 3);
          if (connect_retry_time > MAX_RETRY) {
            connect_retry_time = 0;
            meter_box_list[meter_mission_list[current_mission].meter].error = 4;
            meter_box_list[meter_mission_list[current_mission].box_rank].meter_list[meter_mission_list[current_mission].meter].exc = 1;
            current_mission = -1;
            meter_mission_number--;
          }
        }
        break;
      }
    case METER_TASK_EXC_REPLY : {
        if (meter_receive == meter_should_receive && meter_should_receive != 0) {
          meter_box_list[meter_mission_list[current_mission].box_rank].meter_list[meter_mission_list[current_mission].meter].exc = 0;
          meter_box_list[meter_mission_list[current_mission].box_rank].error = 0;
          connect_retry_time = 0;
          char logs[150];
          sprintf(logs, "%lu : Mission %d execute success ! METER %d in BOX %d has been switched",
                  systime, current_mission, meter_mission_list[current_mission].meter, meter_mission_list[current_mission].box_rank);
          //Serial.println(logs);
          current_mission = -1;
          meter_mission_number--;
          saveSystemRecord(logs);
          meter_task_step = METER_TASK_CONNECT_TO_BOX;
        }
        else {
          // receive error
          Serial.println("--Mission-- Current Mission Fail. No reply");
          connect_retry_time++;
          connectionStop();
          meter_task_step = METER_TASK_CONNECT_TO_BOX;
          selfNextDutyDelay(OS_ST_PER_100_MS * 3);
          if (connect_retry_time > MAX_RETRY) {
            connect_retry_time = 0;
            meter_box_list[meter_mission_list[current_mission].meter].error = 4;
            meter_box_list[meter_mission_list[current_mission].box_rank].meter_list[meter_mission_list[current_mission].meter].exc = 1;
            //meter_box_list[meter_mission_list[current_mission].box_rank].meter_list[meter_mission_list[current_mission].meter].coil = meter_mission_list[current_mission].coil;
            current_mission = -1;
            meter_mission_number--;
          }
        }
        meter_receive = 0;
        meter_should_receive = 0;

        break;
      }
    default: {
        break;
      }
  }
  return 1;
}

int meterProcessReset() {
  meterConn.flush();
  if (connection_status != CONN_DISCONNECT) meterConn.stop();
  connection_status = CONN_DISCONNECT;
  meter_receive = 0;
  meter_should_receive = 0;
}

unsigned int meterListeningTask(int opt) {
  if (connection_status == CONN_INIT) {
    //Serial.print("--MeterListening-- Hardware Connection Status : ");
    //Serial.println(meterConn.checkConnectionN());
    if (meterConn.checkConnectionN() == 1) {
      connection_status = CONN_CONNECTED;
      //Serial.println("Conect to meter Success !");
      taskNextDutyDelay(meter_main_task, 0);
    }
  }
  else if (connection_status == CONN_DISCONNECT) {
    while (meterConn.available()) {
      char c = meterConn.read();
    }
  }
  else if (connection_status == CONN_CONNECTED) {
    if (meter_should_receive - meter_receive > 0) {
      int temp_receive = meterConn.available();
      if (meter_receive + temp_receive >= meter_should_receive) {
        temp_receive = meter_should_receive - meter_receive;
        taskNextDutyDelay(meter_main_task, 0);
      }
      meterConn.read(meter_receive_data + meter_receive, temp_receive);
      meter_receive += temp_receive;
      /*
        if (meter_should_receive == meter_receive){
        Serial.print("--MeterListenning-- In Buffer: ");
        for (int i=0;i<meter_receive;i++){
          Serial.print(meter_receive_data[i],HEX);
          Serial.print(" ");
        }
        Serial.println(" -- ");
        }
      */
    }
    while (meterConn.available() && meter_should_receive - meter_receive <= 0) {
      char c = meterConn.read();
    }
  }
  return 1;
}

unsigned int meterTRTask(int opt) {
  if (systime >= nextMidnight) {
    initHistoryTime();
  }

  if (new_price_tariff.init_time > 0 && new_price_tariff.init_time <= systime){
    price_tariff = new_price_tariff;
    char logs[80];
    sprintf(logs, "%lu : New tariff activied.", systime);
    saveSystemRecord(logs);
    new_price_tariff.init_time = 0;
    saveFlashTariff();
    meterMaintenanceTask(1); // save record
  }

  long old_price = current_price;
  if ((systime > (price_tariff.price1_start_time + thisMidnight)) &&
      (systime < (price_tariff.price2_start_time + thisMidnight)))
  {
    current_price = price_tariff.price1;
  }
  else {
    current_price = price_tariff.price2;
  }
  if (old_price != current_price) taskNextDutyDelay(meter_maintenance_task, 0);
  return 0;
}

unsigned int meterMaintenanceTask(int opt) {
  calculateCredit();
  //saveFlashRecord(0);
  saveHistoryData();
  //saveCreditData();
  return 0;
}

int readAmp(int rank) {
  byte data[12] = {0x00, 0x05, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x00, 0x00, 0x02};
  uint16_t addr = 201 + rank * 2;
  //Serial.print("--ReadAmp-- Address : ");
  //Serial.print(addr);
  //Serial.print(" -- ");
  byte *p = (byte *)&addr;
  data[9] = p[0];
  data[8] = p[1];
  //Serial.print(data[8], HEX);
  //Serial.print(" ");
  //Serial.println(data[9], HEX);
  if (meterConn.write(data, 12) == 12) {
    meter_should_receive = 13;
    //Serial.print("--Read Amp function finished, Address : "); Serial.println(addr);
    return 1;
  }
  else {
    //Serial.print("--Read Amp function finished, Address : "); Serial.println(addr);
    return 0;
  }
}

int readWatt(int rank) {
  byte data[12] = {0x00, 0x05, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x00, 0x00, 0x02};
  uint16_t addr = 221 + rank * 2;
  //Serial.print("--ReadWatt-- Address : ");
  //Serial.print(addr);
  //Serial.print(" -- ");
  byte *p = (byte *)&addr;
  data[9] = p[0];
  data[8] = p[1];
  //Serial.print(data[8], HEX);
  //Serial.print(" ");
  //Serial.println(data[9], HEX);
  if (meterConn.write(data, 12) == 12) {
    meter_should_receive = 13;
    //Serial.print("--Read watt function finished, Address : "); Serial.println(addr);
    return 1;
  }
  else {
    //Serial.print("--Read watt function finished, Address : "); Serial.println(addr);
    return 0;
  }
}

int readKwh(int rank) {
  byte data[12] = {0x00, 0x05, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x00, 0x00, 0x02};
  uint16_t addr = 241 + rank * 2;
  //Serial.print("--ReadKwh-- Address : ");
  //Serial.print(addr);
  //Serial.print(" -- ");
  byte *p = (byte *)&addr;
  data[9] = p[0];
  data[8] = p[1];
  //Serial.print(data[8], HEX);
  //Serial.print(" ");
  //Serial.println(data[9], HEX);
  if (meterConn.write(data, 12) == 12) {
    meter_should_receive = 13;
    //Serial.print("--Read kwh function finished, Address : "); Serial.println(addr);
    return 1;
  }
  else {
    //Serial.print("--Read kwh function finished, Address : "); Serial.println(addr);
    return 0;
  }
}

int switchCoil(int box, int rank, uint8_t flag) {
  byte data[12] = {0x00, 0x05, 0x00, 0x00, 0x00, 0x06, 0x01, 0x05, 0x01, 0x2C, 0xff, 0x00};
  if (flag > 0) data[10] = 0xff;
  else data[10] = 0x00;
  if (charge_controller_status != CHARGE_CONTROLLOR_VOLTAGE_NORMAL) {
    char logs[90];
    Serial.println("--Switch coil-- Trun Coil Off, battery voltage abnormal.");
    sprintf(logs, "%ld : METER %d in BOX %d be switched off, battery voltage low.", systime, meter_rolling + 1, connected_box_id);
    saveSystemRecord(logs);
    data[10] = 0x00;
  }

  uint16_t addr = rank + 299;
  //Serial.print("--CheckCoil-- Address : ");
  //Serial.print(addr);
  //Serial.print(" -- ");
  byte *p = (byte *)&addr;
  data[9] = p[0];
  data[8] = p[1];
  //Serial.print(data[8], HEX);
  //Serial.print(" ");
  //Serial.println(data[9], HEX);
  if (meterConn.write(data, 12) == 12) {
    meter_should_receive = 12;
    //Serial.print("--Switch coil function finished, Address : "); Serial.println(addr);
    return 1;
  }
  else {
    //Serial.print("--Switch coil function finished, Address : "); Serial.println(addr);
    return 0;
  }
}

int calculateCredit() {

  int exe = -1;

  for (int box = 0; box < meter_box_number; box++) {
    for (int meter = 0; meter < METER_PER_BOX; meter++) {

      if (meter_box_list[box].meter_list[meter].kwh == 0) {
        meter_box_list[box].meter_list[meter].kwh = temp_kwh_list[box * METER_PER_BOX + meter];
        if (temp_kwh_list[box * METER_PER_BOX + meter] > 0) {
          exe = 1;
        }
        continue;
      }

      long gap = temp_kwh_list[box * METER_PER_BOX + meter] - meter_box_list[box].meter_list[meter].kwh;
      if (gap > 0) {
        exe = 1;
        meter_box_list[box].meter_list[meter].credit -= gap * current_price;
        meter_box_list[box].meter_list[meter].kwh = temp_kwh_list[box * METER_PER_BOX + meter];
        if (meter_box_list[box].meter_list[meter].credit <= 0) {
          if (meter_box_list[box].meter_list[meter].flag == METER_STATUS_POWER_ON) {
            meter_box_list[box].meter_list[meter].exc = 1;
            meter_box_list[box].meter_list[meter].flag = METER_STATUS_NO_CREDIT_OFF;
            char logs[80];
            sprintf(logs, "%lu : Switch OFF METER %d in BOX %d as no Credit.", systime, meter + 1, meter_box_list[box].id);
            saveSystemRecord(logs);
            addMeterMission(box, meter, 0);
          }
        }
        else {
          if (meter_box_list[box].meter_list[meter].flag == METER_STATUS_NO_CREDIT_OFF) {
            //meter_box_list[box].meter_list[meter].exc = 1;
            //meter_box_list[box].meter_list[meter].flag = METER_STATUS_POWER_OFF;
          }
        }
      }
    }
  }

  if (exe == 1) {
    saveFlashRecord(0);
  }

  return exe;
}

uint32_t stringfloat2longAmp1000(uint8_t *data) {
  float de = 0.0;
  uint8_t *p = (uint8_t *)&de;
  p[0] = data[1];
  p[1] = data[0];
  p[2] = data[3];
  p[3] = data[2];
  //Serial.print(" -- ** Num data fetch : ");
  //Serial.println(de);
  return (uint32_t)(de * 1000);
}

void connectToMeterRank(int i) {
  if (connection_status != CONN_DISCONNECT) meterConn.stop();
  uint8_t *ips = meter_box_list[i].ip;
  IPAddress box_ip(ips[0], ips[1], ips[2], ips[3]);
  char temps[200];
  sprintf(temps, "--METER CONNECT--    %d %hhu.%hhu.%hhu.%hhu:%u", meter_box_list[i].id, meter_box_list[i].ip[0],
          meter_box_list[i].ip[1], meter_box_list[i].ip[2], meter_box_list[i].ip[3], meter_box_list[i].port);
  //Serial.println(temps);
  connected_box_id = meter_box_list[i].id;
  connected_box_rank = i;
  connection_status = CONN_INIT;
  meterConn.connectN(box_ip, meter_box_list[i].port);
  selfNextDutyDelay(OS_ST_PER_SECOND * 2);
}

void connectToMeterID(int id) {
  int i;
  if (connection_status != CONN_DISCONNECT) meterConn.stop();
  for (i = 0; i < meter_box_number; i++) {
    if (meter_box_list[i].id == id) {
      uint8_t *ips = meter_box_list[i].ip;
      char temps[100];
      sprintf(temps, "--METER CONNECT--    %d %hhu.%hhu.%hhu.%hhu:%u", meter_box_list[i].id, meter_box_list[i].ip[0],
              meter_box_list[i].ip[1], meter_box_list[i].ip[2], meter_box_list[i].ip[3], meter_box_list[i].port);
      //Serial.println(temps);
      IPAddress box_ip(ips[0], ips[1], ips[2], ips[3]);
      connection_status = CONN_INIT;
      connected_box_id = meter_box_list[i].id;
      connected_box_rank = i;
      meterConn.connectN(box_ip, meter_box_list[i].port);
      selfNextDutyDelay(OS_ST_PER_SECOND);
      break;
    }
  }
}

void connectionStop() {
  meterConn.stop();
  connection_status = CONN_DISCONNECT;
  connected_box_id = -1;
  connected_box_rank = -1;
  meter_should_receive = 0;
  meter_receive = 0;
}

int saveHistoryData() {
  if (systime < 1200000000) {
    Serial.println(" -- Histroy Data -- No System Time , Can't record history !");
    return -1;
  }

  if (sd_status != SD_EXIST) return 0;

  if (thisMidnight == 0 || systime >= nextMidnight) {
    initHistoryTime();
  }

  char fileName[20];
  sprintf(fileName, "record/%ld.log", (int)(thisMidnight / 100));
  File history = openSdFile(fileName, FILE_WRITE);
  if (!history) return -1;

  history.print(systime);
  history.print(" *");
  int i = 0;
  history.print(meter_box_number);
  history.print(" |");
  for (; i < meter_box_number; i++) {
    history.print(meter_box_list[i].id);
    history.print(" : ");
    for (int j = 0; j < METER_PER_BOX; j++) {
      char temp[200];
      sprintf(temp, "%lu %lu %lu %lu %hhu , ",
              meter_box_list[i].meter_list[j].amp,
              meter_box_list[i].meter_list[j].watt,
              meter_box_list[i].meter_list[j].kwh,
              meter_box_list[i].meter_list[j].credit,
              meter_box_list[i].meter_list[j].flag
             );
      history.write(temp);
    }
  }
  history.print("##\n");
  history.flush();
  history.close();

  saveCreditData();
/*
  sprintf(fileName, "/credit/%ld.log", thisMidnight / 100);
  File creditFile = openSdFile(fileName, FILE_WRITE);
  creditFile.print(systime);
  creditFile.print(" *");
  creditFile.print(meter_box_number);
  creditFile.print(" |");
  for (i = 0; i < meter_box_number; i++) {
    creditFile.print(meter_box_list[i].id);
    creditFile.print(" : ");
    for (int j = 0; j < METER_PER_BOX; j++) {
      creditFile.print(meter_box_list[i].meter_list[j].credit);
      creditFile.print(" , ");
    }
  }
  creditFile.print("##\n");
  creditFile.flush();
  creditFile.close();
*/

  return 1;
}

int saveCreditData() {
  if (systime < 1200000000) {
    Serial.println(" -- Credit Data -- No System Time , Can't record Credit history !");
    return -1;
  }

  if (sd_status != SD_EXIST) return 0;

  if (thisMidnight == 0 || systime >= nextMidnight) {
    initHistoryTime();
  }
  char fileName[20];
  int i;
  sprintf(fileName, "credit/%ld.log", thisMidnight / 100);
  File creditFile = openSdFile(fileName, FILE_WRITE);
  if (!creditFile) return -1;
  creditFile.print(systime);
  creditFile.print(" *");
  creditFile.print(meter_box_number);
  creditFile.print(" |");
  for (i = 0; i < meter_box_number; i++) {
    creditFile.print(meter_box_list[i].id);
    creditFile.print(" : ");
    for (int j = 0; j < METER_PER_BOX; j++) {
      creditFile.print(meter_box_list[i].meter_list[j].credit);
      creditFile.print(" , ");
    }
  }
  creditFile.print("##\n");
  creditFile.flush();
  creditFile.close();
  return 1;
}

int saveChargeRecord(int box, int meter, uint32_t credit) {
  char fileName[20];
  if (sd_status != SD_EXIST) return 0;
  sprintf(fileName, "charge/%ld.log", thisMidnight / 100);
  File creditFile = openSdFile(fileName, FILE_WRITE);
  if (!creditFile) return -1;
  char output[100];
  sprintf(output, "%lu : Charge %lu to METER %d in BOX %d ##\n", systime, credit, meter, box);
  Serial.println(output);
  creditFile.print(output);
  creditFile.flush();
  creditFile.close();
  return 1;
}

int saveSystemRecord(char *data) {
  Serial.println(data);
  if (sd_status != SD_EXIST) return 0;
  char fileName[20];
  sprintf(fileName, "system/%ld.log", thisMidnight / 100);
  File file = openSdFile(fileName, FILE_WRITE);
  if (!file) return -1;
  file.print(data);
  file.print(" ##\n");
  file.flush();
  file.close();
  return 1;
}

void initHistoryTime() {
  uint32_t ts = systime;
  thisMidnight = ONE_DAY_TIME * (uint32_t)(ts / ONE_DAY_TIME);
  nextMidnight = thisMidnight + ONE_DAY_TIME;
}

int chargeUser(int user_id, long credit) {
  if (user_id < 1 || user_id > METER_PER_BOX * meter_box_number) return -1;
  int box = (user_id - 1) / METER_PER_BOX;
  int meter = user_id - box * METER_PER_BOX - 1;
  return chargeCredit(box, meter, credit);
}

int chargeCredit(int box, int meter, long credit) {
  int i;
  int control = 0;
  long pervious = 0;
  if (box < 0 || box >= meter_box_number) return -1;
  //for (i=0;i<meter_box_number;i++){
  //if (meter_box_list[i].id == box_id){
  pervious = meter_box_list[box].meter_list[meter].credit;
  meter_box_list[box].meter_list[meter].credit += credit;
  if (pervious <= 0 && meter_box_list[box].meter_list[meter].credit > 0) {
    control = 1;
  }
  if (!saveChargeRecord(box, meter, credit)) {
    char logs[100];
    sprintf(logs, "%lu : Charge record save Error, on METER(rank) %d in BOX(rank) %d.", systime, meter, box);
    saveSystemRecord(logs);
  }
  if (!saveCreditData()) {
    char logs[100];
    sprintf(logs, "%lu : Save Credit Data Error, on METER(rank) %d in BOX(rank) %d.", systime, meter, box);
    saveSystemRecord(logs);
  }

  if (control == 1 && meter_box_list[box].meter_list[meter].flag == METER_STATUS_NO_CREDIT_OFF) {
    meter_box_list[box].meter_list[meter].flag = METER_STATUS_POWER_ON;
    addMeterMission(box, meter, 1);
    char logs[120];
    sprintf(logs, "%lu : Switch ON METER(rank) %d in BOX(rank) %d as Credit charged to positive.", systime, meter, box);
    saveSystemRecord(logs);
  }
  // start record task
  //taskNextDutyDelay(meter_maintenance_task,0);
  saveFlashRecord(0);
  saveHistoryData();
  return 1;
}

int chargeVendor(int id, long credit) {
  if (id < 1 || id > vendor_number) return -1;
  vendor_credit_list[id - 1] += credit;
  uint8_t *ps = (uint8_t *)vendor_credit_list;
  for (int i=0;i<sizeof(long) * VENDOR_NUMBER;i++){
    storageWriteByte(VENDOR_CREDIT_ADDR+i,ps[i]);
  }
  //storageWriteBytes(VENDOR_CREDIT_ADDR, (byte *)ps, sizeof(long) * VENDOR_NUMBER);
  return 1;
}

int addMeterMission(int box, int meter, int st) {
  if (box >= meter_box_number) return -1;
  if (meter >= METER_PER_BOX || meter < 0) return -1;
  if (st != 1 && st != 0) return -1;
  if (meter_mission_number >= MAX_MISSION_NUMBER) return -1;
  meter_mission_list[meter_mission_number].box_rank = box;
  meter_mission_list[meter_mission_number].meter = meter;
  meter_mission_list[meter_mission_number].coil = st;
  meter_mission_number++;
  return 1;
}

int switchUserStatus(int id, uint8_t st) {
  int meter_id;
  int meter_box;
  char logs[120];

  if (id > 10 && id <= METER_PER_BOX * meter_box_number) {
    meter_box = (id - 1) / 10;
    meter_id = id - meter_box * 10 - 1;
  }
  else if (id <= 10 && id > 0) {
    meter_box = 0;
    meter_id = id - 1;
  }
  else {
    return -1;
  }

  sprintf(logs, "--Control-- %lu -- Switch box : %d ,Meter : %d, status : %d\n", systime, meter_box, meter_id, st);


  if (st == 1) {
    if (meter_box_list[meter_box].meter_list[meter_id].credit > 0) {
      meter_box_list[meter_box].meter_list[meter_id].flag = 1;
      addMeterMission(meter_box, meter_id, 1);
    }
    else {
      meter_box_list[meter_box].meter_list[meter_id].flag = 2;
    }
  }
  else if (st == 2) {
    meter_box_list[meter_box].meter_list[meter_id].flag = 3;
    addMeterMission(meter_box, meter_id, 0);
  }

  saveSystemRecord(logs);
  saveHistoryData();
  saveFlashRecord(0);
  return 1;
}

int updateTariff(uint32_t t1, long p1, uint32_t t2, long p2,uint32_t gen_time,uint32_t init_time) {
  if (t1 >= 0 && t2 > 0 && t1 < t2 && p1 >= 0 && p2 >= 0) {
    price_tariff.price1_start_time = t1;
    price_tariff.price1 = p1;
    price_tariff.price2_start_time = t2;
    price_tariff.price2 = p2;
    price_tariff.generate_time = gen_time;
    price_tariff.init_time = init_time;
    saveFlashTariff();
    return 1;
  }
  else {
    return 0;
  }
}

int getBoxRankByID(int id) {
  for (int i = 0; i < meter_box_number; i++) {
    if (meter_box_list[i].id == id) {
      return i;
    }
  }
  return -1;
}

/******************************************* Meter Control End*****************************************************/

/******************************************* Payment System Start *************************************************/


int paymentQueryUserCallback(Payment_User_Type *user) {
  int i;
  //if (user->id_len != SN_LENGTH + 5) return -1;
  for (i = 0; i < SN_LENGTH; i++) {
    if (user->user_id[i] != mg_sn[i]) {
      Serial.println("--Payment-- MG_SN Error !");
      return -1;
    }
  }

  int id_number = 0;
  for (int j = 0;j<4;j++){
    char c = user->user_id[++i];
    if (!isdigit(c)) return -1;
    id_number = id_number*10 + (c - '0');
  }

  Serial.print("--Payment-- Get user number ! : ");Serial.println(id_number);

  if (user->user_id[16] == 'V' && id_number<=vendor_number){
    user->balence = vendor_credit_list[id_number];
    user->rank = id_number;
    return 1;
  }
  else if (user->user_id[16] == 'U' && id_number <= user_number ){
    user->rank = id_number;
    int box = (id_number-1) / METER_PER_BOX;
    int meter = (id_number-1) - METER_PER_BOX*box;
    user->balence = meter_box_list[box].meter_list[meter].credit;
    return 1;
  }
  else{
    return -1;
  }
}

int paymentTopUpUserCallback(Payment_Top_Up_User_Type *topup) {
  Payment_User_Type vendor,user;
  vendor.user_id = topup->vendor_id;
  vendor.id_len = topup->id_len;
  user.user_id = topup->user_id;
  user.id_len = topup->id_len;
  if (paymentQueryUserCallback(&vendor) == -1){
    return PAYMENT_TOPUP_NO_VENDOR;
  }
  if (paymentQueryUserCallback(&user) == -1){
    return PAYMENT_TOPUP_NO_USER;
  }
  if (vendor.balence < topup->credit){
    return PAYMENT_TOPUP_NO_CREDIT;
  }
  if (chargeVendor(vendor.rank+1,0 - topup->credit)){
    chargeUser(user.rank,topup->credit);
  }
  return 1;
}

int paymentTopUpVendorCallback(Payment_Top_Up_Vendor_Type *topup) {
  Payment_User_Type vendor;
  vendor.user_id = topup->vendor_id;
  vendor.id_len = topup->id_len;
  if (paymentQueryUserCallback(&vendor) == -1){
    Serial.println("Topup Vendor : Vendor ID error");
    return PAYMENT_TOPUP_NO_VENDOR;
  }
  int ck = checkPaymentRecordCallback(&(topup->record));
  if (ck != 1) return ck;

  //savePaymentRecord(&(topup->record));
  chargeVendor(vendor.rank+1,topup->credit);
  return 1;
}

int paymentUpdateTariffCallback(Payment_Tariff_Type *tariff) {
  if (tariff->generate_time < new_price_tariff.generate_time ||
      tariff->generate_time < price_tariff.generate_time ||
      tariff->init_time < new_price_tariff.init_time ||
      tariff->init_time < price_tariff.init_time
  ){
    return -1;
  }
  new_price_tariff.price1_start_time = tariff->price1_start_time;
  new_price_tariff.price2_start_time = tariff->price2_start_time;
  new_price_tariff.price1 = tariff->price1;
  new_price_tariff.price2 = tariff->price2;
  new_price_tariff.generate_time = tariff->generate_time;
  new_price_tariff.init_time = tariff->init_time;
  saveFlashTariff();
  char logs[230];
  sprintf(logs,"--Payment-- Get new tariff, generate time: %lu , init time: %lu, time1: %lu, price1: %lu, time2: %lu, price2: %lu",
          new_price_tariff.generate_time, new_price_tariff.init_time, new_price_tariff.price1_start_time, new_price_tariff.price1,
          new_price_tariff.price2_start_time, new_price_tariff.price2);
  saveSystemRecord(logs);
  return 1;
}

int paymentCheckInfoCallback(char *data, int len,byte type) {
  uint16_t i,j,k;
  data[0] = 0;
  sprintf(data,"%lu - ",systime);
  for (i=0;i<meter_box_number;i++){
    for (j=0;j<METER_PER_BOX;j++){
      k = strlen(data);
      if (k>len) return k;
      sprintf(data+k,"%lu,%lu ",meter_box_list[i].meter_list[j].kwh,meter_box_list[i].meter_list[j].credit);
    }
  }
  return strlen(data);
}


int paymentCheckTimeCallback(uint32_t *ts){
  if (systime > 1000000000){
    *ts = systime;
    return 1;
  }
  return -1;
}

int checkPaymentRecordCallback(Payment_Record_Type *record){
  return 1;
  uint8_t number = storageReadByte(PAYMENT_NUMBER_ADDR);
  uint8_t index = storageReadByte(PAYMENT_INDEX_ADDR);
  int i = 0;
  char old[20];
  char *p = (char *)record;

  if (systime - record->ts > 3600L*24*60){
    Serial.print("Credit Check : Time Exp.  sys: ");
    Serial.print(systime);
    Serial.print("  record: ");
    Serial.print(record->ts);
    Serial.print("  gap: ");
    Serial.println(systime - record->ts);
    return PAYMENT_TOPUP_CREDIT_EXPIRED;
  }

  for (i=0;i<number;i++){
    uint16_t u_start = PAYMENT_RECORD_ADDR + i*PAYMENT_RECORD_LEN;
    int ck = 0;
    for (int j = 0;j<PAYMENT_RECORD_LEN - 4;j++){
      if (p[j] == storageReadByte(u_start+j)){
        ck++;
      }
    }
    if (ck>=16) {
      Serial.println("Credit Check : Credit UUID used.");
      return PAYMENT_TOPUP_CREDIT_USED;
    }
  }
  return 1;
}

int savePaymentRecord(Payment_Record_Type *record){
  return 1;
  uint8_t number = storageReadByte(PAYMENT_NUMBER_ADDR);
  uint8_t index = storageReadByte(PAYMENT_INDEX_ADDR);
  if (number < PAYMENT_RECORD_NUMBER){
    number++;
    storageWriteByte(PAYMENT_NUMBER_ADDR,number);
  }

  char *cp = (char *)record;
  uint32_t head = PAYMENT_RECORD_ADDR+index*sizeof(Payment_Record_Type);
  for (int i=0;i<sizeof(Payment_Record_Type);i++){
    storageWriteByte(head+i,cp[i]);
  }
  //storageWriteBytes((uint32_t)(PAYMENT_RECORD_ADDR+index*sizeof(Payment_Record_Type)),(byte *)record,sizeof(Payment_Record_Type));
  if (++index >= PAYMENT_RECORD_NUMBER) index = 0;
  storageWriteByte(PAYMENT_INDEX_ADDR,index);
  return 1;
}


/******************************************* Payment System End *************************************************/


void setup() {
  Serial.begin(115200);
  Serial.println("System initializing ... ");
  pinMode(46, OUTPUT);
  digitalWrite(46, LOW);
  ethernetReboot();
  Wire.begin();

  /* initialize system time */
  systime = 0;
#ifdef __RTC__
  rtcInit();
  systime = readRTC();
  sys_start_time = systime;
#endif

  /* initialize lightOS */
  osSetup();
  Serial.println("OS Setup OK !");

  /* initialize SD card */

  if (!sdCardInit()) {
    Serial.println("\nSD Card Init Fail.");
  }
  else {
    Serial.println("\nSD Card Init Success.");
  }

  /* initialize meter objects*/
  readInitCommand();
  printMeterObjects();
  Serial.println("Init Meter Object Success !");

  /* initialize the Ethernet adapter */
  Ethernet.begin(mac, ip_self, gateway, gateway, subnet);
  Serial.println("\nEthernet Init Success.");

  /* initialize the tft screen */
#ifdef __TFT_28_SCREEN__
  tft_screen_init();
#endif

#ifdef __UTP_TIME__
  Udp.begin(localPort);
  systime = syncNtpTime();
#endif
  initHistoryTime();
  Serial.println("\nTime Init Success.");
  //systemErrorHandler(1);

  /* initialize web server */
#ifdef __WEB_SERVER__
  webServerInit();
  Serial.println("Web Server Init Success !");
#endif

  /* initialize application task */
  //taskRegister(systemMaintain, OS_ST_PER_SECOND * 120, 1, OS_ST_PER_SECOND * 120);
  meter_main_task = taskRegister(meterMainTask, 1, 1, OS_ST_PER_SECOND);
  taskRegister(sysCheckTime, OS_ST_PER_SECOND * 60, 1, 0);
  meter_maintenance_task = taskRegister(meterMaintenanceTask, OS_ST_PER_SECOND * 600, 1, OS_ST_PER_SECOND * 60);
  cc_main_task = taskRegister(ccMainTask, 0, 1, OS_ST_PER_SECOND);

#ifdef __TFT_28_SCREEN__
  taskRegister(screenUpdate, OS_ST_PER_SECOND, 1, 0);
#endif
  paymentSystemInit();
  Serial.println("All Task Setup Success !");

  /* initialize hardware timer */
  Timer2.attachInterrupt(onDutyTime);
  //Timer3.initialize(5000); // Calls every 5ms
  Timer2.start(5000);
  watchdogSetup();
  //update time
  systime = readRTC();
  char logs[200];
  sprintf(logs,"%lu : System finish init ",systime);
  saveSystemRecord(logs);
  Serial.print("Flash Storage Size : ");Serial.println(IFLASH1_SIZE);
}
