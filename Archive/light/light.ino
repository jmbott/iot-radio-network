
#include "lightOS.h"
#include "Timer.h"

// Timer name
Timer t;

#define OS_TASK_LIST_LENGTH 6

#define LED 13

//****task variables****//
OS_TASK *LED_OFF;
OS_TASK *LED_ON;

// system time
unsigned long systime = 0;
unsigned long sys_start_time = 0;

/*********************************** LED on ************************************/

unsigned int led_on(int opt) {
  digitalWrite(LED, HIGH);
  Serial.println("LED ON!");
  return 0;
}

/********************************* LED on end **********************************/
/*********************************** LED off ***********************************/

unsigned int led_off(int opt) {
  digitalWrite(LED, LOW);
  Serial.println("LED OFF!");
  return 0;
}

/********************************* LED off end *********************************/


void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("System initializing ... ");
  
  // LED setup
  pinMode(LED, OUTPUT);

  /* initialize lightOS */
  osSetup();
  Serial.println("OS Setup OK !");

  /* initialize application task
  OS_TASK *taskRegister(unsigned int (*funP)(int opt),unsigned long interval,unsigned char status,unsigned long temp_interval)
  taskRegister(FUNCTION, INTERVAL, STATUS, TEMP_INTERVAL); */
  LED_OFF = taskRegister(led_off, OS_ST_PER_SECOND*10, 1, 0);
  LED_ON = taskRegister(led_on, OS_ST_PER_SECOND*20, 1, 0);

  // ISR or Interrupt Service Routine for async
  t.every(5, onDutyTime);  // Calls every 5ms

  Serial.println("All Task Setup Success !");

}

void loop() {
  os_taskProcessing();
  constantTask();
}

void constantTask() {
  t.update();
  //led_on(0);
}

// support Function onDutyTime: support function for task management
void onDutyTime() {
  static int a = 0;
  a++;
  if (a >= 200) {
    systime++;
    a = 0;
    Serial.print("Time Now is : "); Serial.println(systime);
  }
  _system_time_auto_plus();
}

extern void _lightOS_sysLogCallBack(char *data) {
  Serial.print("--LightOS--  "); Serial.print(data);
}

