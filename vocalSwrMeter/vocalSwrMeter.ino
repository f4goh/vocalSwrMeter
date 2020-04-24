/**********************************************************
  f4goh-kb1goh April 2020 cc-by-nc-sa
  SWR meter with vocal features
  Hardware: Serial MP3 Player
  Board:  Arduino UNO R3
  mp3 player YX5300 catalex
  create folder 02 and samples voices files
file name  Information time (ms)
000.mp3 zero  583
001.mp3 one 432
002.mp3 two 285
003.mp3 three 412
004.mp3 four  482
005.mp3 five  583
006.mp3 six 607
007.mp3 seven 578
008.mp3 eight 409
009.mp3 nine  569
010.mp3 ten 357
011.mp3 power 493
012.mp3 watt  418
013.mp3 manual mode 969
014.mp3 automatic mode  1283
015.mp3 swr too high  2090
016.mp3 swr 1 point 2461
017.mp3 swr 2 point 2206
018.mp3 swr 3 point 2171
019.mp3 swr 4 point 2252
020.mp3 swr 5 point 2218
021.mp3 qrt 723
022.mp3 power on  789

**********************************************************/
//SWR meter configuration

//#define LANGUAGE_FR
#define LANGUAGE_EN

//#define DEBUG_SWR
//#define DEBUG_DBM

#define PERSONAL_CALIBRATION

#define OffsetdBmBridge 45

#include <SoftwareSerial.h>

#define ARDUINO_RX 5  //should connect to TX of the Serial MP3 Player module
#define ARDUINO_TX 6  //connect to RX of the module
#define bpMode 9      //bp mode pin
#define bpmanual 8    //bp manual pin
#define vf A1         //analog direct input from HF bridge
#define vr A0         //analog reverse input from HF bridge

SoftwareSerial mp3(ARDUINO_RX, ARDUINO_TX);

#define CMD_SEL_DEV 0X09
#define DEV_TF 0X02
#define CMD_PLAY 0X0D
#define CMD_PLAY_FOLDER_FILE 0X0F

#ifdef LANGUAGE_FR
#define DIRECTORY 0X0100
int waitSampleDelay[] = {575, 444, 313, 444, 392, 653, 679, 522, 522, 470, 549, 784, 601, 1071, 1202, 1254, 1306, 1411, 1463, 1411, 1463, 679, 1593};
#endif
#ifdef LANGUAGE_EN
#define DIRECTORY 0X0200
int waitSampleDelay[] = {583, 432, 285, 412, 482, 563, 607, 578, 409, 569, 357, 493, 418, 969, 1283, 2090, 2461, 2206, 2171, 2252, 2218, 723, 789};
#endif

enum modes
{
  MANUAL,
  AUTOMATIC
};

unsigned char mode = MANUAL;
unsigned char stateBpMode = 0;
unsigned char stateBpmanual = 0;
float P;

void setup()
{
  Serial.begin(9600);
  pinMode(bpMode, INPUT_PULLUP);
  pinMode(bpmanual, INPUT_PULLUP);
  analogReference(EXTERNAL);        //external reference for lm385-2.5

  mp3.begin(9600);
  delay(500);   //Wait YX5300 mp3 chip initialization complete
  sendCommand(CMD_SEL_DEV, DEV_TF);   //select the TF card
  delay(200);
  playIntro();
  Serial.println("SWR power on");
}

void loop()
{
  char c;
  if (Serial.available() > 0) {
    c = Serial.read();
    Serial.print("Test Voices samples");
    if (c == 't') testVoice();
  }

  checkBouttons();
  if (mode == AUTOMATIC) {
    int swr = mesureSWR();
    if (P >= 1) {
      playPwrSwr(P,swr);
    }
  }
  delay(100);
}

/*
   swapp manual and automatic mode
*/

void checkBouttons() {
  stateBpMode = stateBpMode | digitalRead(bpMode);
  stateBpmanual = stateBpmanual | digitalRead(bpmanual);
  if (stateBpMode == 2) {
    playMode();
  }
  if (stateBpmanual == 2) {
    Serial.println("ask for power/swr measurement");
    int swr = mesureSWR();
    if (P > 1) {
        playPwrSwr(P,swr);
    }
    else {
      Serial.println("no power say qrt");
      unsigned char seq[] = {21};
      playSequence(seq, 1);
    }
  }
  stateBpMode = (stateBpMode << 1) & 3;
  stateBpmanual = (stateBpmanual << 1) & 3;
}

void playPwrSwr(float p, int swr) {
  Serial.print("Pwr: ");
  Serial.println(P);
  delay(300);
  Serial.print("SWR: ");
  Serial.println(swr);
  playSwr(swr);
  playPower(P);
}



/*
   mp3 Sequencer
*/

void testVoice()
{
  //float step=0.1;
  int n;
  for (n = 10; n < 20; n++) {
    Serial.print("Power :");
    Serial.println((n - 10));
    delay(1000);
    playPower((float)(n - 10));
    Serial.print("Swr :");
    Serial.println(n);
    playSwr(n);
    delay(1000);
  }
  playSwr(99);
  playMode();
  playMode();
}

//swapp between manual mode and automatic mode
void playMode() {
  unsigned char seq[1];
  if (mode == MANUAL) {
    Serial.println("Automatic Mode");
    seq[0] = 14;
    mode = AUTOMATIC;
  } else {
    Serial.println("Manual Mode");
    seq[0] = 13;
    mode = MANUAL;
  }
  playSequence(seq, 1);
}

//play swr until 5.9 else say swr too high
void playSwr(int swr) {
  unsigned char unit = swr / 10;
  unsigned char decimal = swr % 10;
  if (swr < 60) {
    unsigned char seq[] = {15 + unit, decimal};
    playSequence(seq, 2);
  }
  else
  {
    unsigned char seq[] = {15};
    playSequence(seq, 1);
  }
}

//simple power play in watts
//for power upper that 9Watts  say "power one five watts" for 15W
void playPower(float pw) {
  if (pw < 10) {
    unsigned char seq[] = {11, round(pw), 12};
    playSequence(seq, 3);
  }
  else {
    unsigned char seq[] = {11, (int)(pw / 10), (int) (pw) % 10, 12};
    playSequence(seq, 4);
  }
}

//say power on and manual mode
void playIntro()
{
  unsigned char seq[] = {22};
  playSequence(seq, 1);
  delay(500);
  seq[0] = 13;
  playSequence(seq, 1);
}

//play sequence recorded in seq unsigned char array
void playSequence(unsigned char seq[], char nb) {
  int n;
  for (n = 0; n < nb; n++) {
    sendCommand(CMD_PLAY_FOLDER_FILE, DIRECTORY | seq[n]);
    delay(waitSampleDelay[seq[n]]);
    //Serial.println(n);    //sequence number
  }
}

//send command to mp3 player
void sendCommand(int8_t command, int16_t dat)
{
  static int8_t Send_buf[8] = {0} ;
  delay(20);
  Send_buf[0] = 0x7e; //starting byte
  Send_buf[1] = 0xff; //version
  Send_buf[2] = 0x06; //the number of bytes of the command without starting byte and ending byte
  Send_buf[3] = command; //
  Send_buf[4] = 0x00;//0x00 = no feedback, 0x01 = feedback
  Send_buf[5] = (int8_t)(dat >> 8);//data high
  Send_buf[6] = (int8_t)(dat); //data low
  Send_buf[7] = 0xef; //ending byte
  for (uint8_t i = 0; i < 8; i++) //send command
  {
    mp3.write(Send_buf[i]) ;
  }
}

/*
   measure power in dbm, watt and compute swr
*/

int mesureSWR(void)
{
  unsigned int fwd_num, sum_fwd;
  unsigned int rev_num, sum_rev;
  int n;
  sum_fwd = 0;
  sum_rev = 0;
  for (n = 0; n < 50; n++) {
    sum_fwd += analogRead(vf); // forward power
    sum_rev += analogRead(vr); // reverse power
    delay(1);
  }
  fwd_num = sum_fwd / 50;
  rev_num = sum_rev / 50;
#ifdef PERSONAL_CALIBRATION
  float fwd_dbm =   NtodBmForward(fwd_num); // compute forward power in dbm (calibrate formula)
  float rev_dbm =   NtodBmReverse(rev_num); // compute reverse power in dbm (calibrate formula)
#else
  float fwd_dbm =   NtodBm(fwd_num); // compute forward power in dbm (theoric formula)
  float rev_dbm =   NtodBm(rev_num); // compute reverse power in dbm (theoric formula)
#endif

  float Rho = 0;
  float swr = 0;

  P = pow(10, fwd_dbm / 10) / 1000; //compute direct power in Watts

#ifdef DEBUG_DBM
  Serial.println("********");
  Serial.print("CAN fwd :");
  Serial.println(fwd_num);
  Serial.print("CAN rev :");
  Serial.println(rev_num);
  Serial.println("dbm---");
  Serial.println(fwd_dbm);
  Serial.println(rev_dbm);
#endif


  if (rev_dbm < fwd_dbm) {    //compute swr
    Rho = (rev_dbm - fwd_dbm) / 20;
    Rho = pow(10, Rho);
    swr = (1 + Rho) / (1 - Rho);
#ifdef DEBUG_SWR
    Serial.println("********");
    Serial.print("rho ");
    Serial.println(Rho);
    Serial.print("swr ");
    Serial.print(swr);
    Serial.print(" / ");
    Serial.print(P);
    Serial.println(" W");
#endif
  }
  else {
    swr = 10; // no antenna or dummy load
  }
  return (int) (swr * 10);
}

//compute power in dbm (theoric formula)
float NtodBm(int N)
{
  return  (100 * (float) N) / 1024 - 90 + OffsetdBmBridge;
}

//compute forward power in dbm (calibrate formula)
float NtodBmForward(int N)
{
  return  0.0958 * (float) N - 42.834;
}

//compute reverse power in dbm (calibrate formula)
float NtodBmReverse(int N)
{
  return  0.0963 * (float) N - 44.258;
}
