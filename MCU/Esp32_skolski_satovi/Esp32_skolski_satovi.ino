

/*
  Esp32 mikrokontroler pokrece skolske satove i pali zvono u odredjenim trenucima. Komunikacija sa Android uredjajem preko WiFi za dobijanje sablona po kojem se zvoni i jos neke info. Postoje i tasteri za podesavanje satova.
  Za visoku preciznost merenja vremena koriste se 2 pristupa: povezivanje na lokalnu internet mrezu zarad dobijanja date/time informacije. Takodje vreme se meri unutrasnjim tajmerima. 
  Info sa interneta se moze koristiti za korekciju i sinhronizaciju.
  Mehanizam kazaljke radi tako sto se promene stanja 2 pina (toggle). Jedanput pin1 promena na HIGH i pin2 promena na LOW, pa onda obrnuto kad bude sledeca minuta. U medjuvremenu oba pina su ugasena zbog ustede.

  Softver: Nemanja M.
  Hardver: Dragan M.
  Decembar 2024.

*/

#include <WiFi.h>
#include <string>
#include <time.h>

using namespace std;


#define PIN_KAZALJKA1 33
#define PIN_KAZALJKA2 12
#define PIN_TASTER1 4
#define PIN_TASTER2 18
#define PIN_ZVONO 19

#define MAX_DUZINA 25
#define BR_POKUSAJA_POVEZIVANJA 20
#define DEBOUNCING_INTERVAL 10000

#define STATUS_OK 0   // sve je u redu
#define STATUS_ERR_UNKNOWN_TIME 1 // neuspelo povezivanje pri paljenju sistema, ne zna vreme
#define STATUS_WARNING_TIME_NOT_CORRECTED 2 // neuspelo povezivanje pri korekciji vremena

typedef struct{       // struktura koja opisuje vremensku odrednicu, sve info o datumu i vremenu, azurira se preko tajmera a korekcija se vrsi sa interneta
  uint8_t sek;
  uint8_t min;
  uint8_t sat;
  uint8_t dan;
  uint8_t dan_u_nedelji;
  uint8_t mesec;
  uint16_t god;
} Vreme;

// AP - Access Point - na njega se povezuju
// STA - Station - povezuje se na druge AP

const char* ssid_AP = "SAT";     
const char* password_AP = "123";

char ssid_STA[MAX_DUZINA] = "123456";
char password_STA[MAX_DUZINA] = "tisovac2";

char NTP_server[MAX_DUZINA] = "pool.ntp.org";
const long gmtPomak = 3600U;       // Pomak u sekundama od GMT (za CET 3600)
const int letnje_aktivno = 3600U;   // Letnje računanje vremena (ako je aktivno)

String str_za_wifi = "";

uint8_t br_sekundi = 0U; // brojac prekida tajmera
Vreme v;
bool sinhronizovan = false;
bool prekid_tajmera = false;
bool prekid_tastera1 = false;
bool prestupna_godina = false;
uint8_t status_sistema = STATUS_ERR_UNKNOWN_TIME;   // varijabla koja govori u kom stanju se nalazi sistem
bool povezan = false;
bool pin1_na_low = true; // info da pamti koji pin treba da se ugasi a koji da se upali kad dodje vreme da se posalje signal kazaljki sata. U medjuvremenu su ugasena oba pina zbog ustede. Signalizacija traje x ms.
uint8_t br_prekida_tajmera = 0U;
uint16_t br_debouncing_taster1 = 0U;

hw_timer_t* timer0 = NULL; 
portMUX_TYPE timerMux0 = portMUX_INITIALIZER_UNLOCKED;

portMUX_TYPE muxTaster1 = portMUX_INITIALIZER_UNLOCKED;                  //za external interrupt
portMUX_TYPE muxTaster2 = portMUX_INITIALIZER_UNLOCKED; 

void IRAM_ATTR onTimer0();    // prekidne rutine unutrasnjeg tajmera i spoljasnjih tastera
void IRAM_ATTR handleExternalTaster1();
void IRAM_ATTR handleExternalTaster2();
void vremenski_pomak();     // na osnovnu vremenskih info (sek, min, sat...) pomera ostale vrednosti u skladu sa pravilima
void sinhronizacija();   // povezivanje na internet server i sinhronizacija sa vremenskom zonom, korekcija akumulirane greske, jednom dnevno
void posalji_impuls();  // postavlja stanja pinova tako da pokrene mehanizam kazaljke sata



void setup() {
  // put your setup code here, to run once:
  setCpuFrequencyMhz(80);
  Serial.begin(115200);

  timer0 = timerBegin(0, 80U, true);
  timerAttachInterrupt(timer0, &onTimer0, true);
  timerAlarmWrite(timer0, 1000000, true);     // korak od 1sek
  timerAlarmEnable(timer0);

  pinMode(PIN_TASTER1, INPUT_PULLUP);    //za spoljasnji prekid, taster
  attachInterrupt(digitalPinToInterrupt(PIN_TASTER1), handleExternalTaster1, RISING);

  pinMode(PIN_TASTER2, INPUT_PULLUP);    //za spoljasnji prekid, taster
  attachInterrupt(digitalPinToInterrupt(PIN_TASTER2), handleExternalTaster2, RISING);

  pinMode(PIN_KAZALJKA1, OUTPUT);   // 2 pina za upravljanje kazaljkama
  digitalWrite(PIN_KAZALJKA1, LOW);

  pinMode(PIN_KAZALJKA2, OUTPUT);   
  digitalWrite(PIN_KAZALJKA2, HIGH);

  v.sek = 0U;
  v.min = 0U;
  v.sat = 0U;
  v.dan = 1U;
  v.dan_u_nedelji = 1U;    // 0 - nedelja, 1 - pon, ..., 6 - subota
  v.mesec = 1U;
  v.god = 2025U;

  WiFi.mode(WIFI_AP_STA);   // hibridni rezim - istovremeno stanica i pristupna tacka (povezuje se na druge WiFi mreze, a i drugi se mogu povezati na njenu WiFi mrezu)
  WiFi.begin(ssid_STA, password_STA); // inicijalizacija WiFi

  uint8_t br_pokusaja = 0U;
  while(WiFi.status() != WL_CONNECTED && br_pokusaja < BR_POKUSAJA_POVEZIVANJA){   // povezivanje na WiFi mrezu (koja ima internet)
    Serial.write(".");
    br_pokusaja += 1U;
    delay(500);
  }
  if(br_pokusaja < BR_POKUSAJA_POVEZIVANJA){
    povezan = true;
  }
  Serial.println(WiFi.localIP());
  WiFi.softAP(ssid_AP, password_AP);  // inicijalizacija sopstvene WiFi mreze

  delay(1000U);

  
  configTime(gmtPomak, letnje_aktivno, NTP_server);

  
  struct tm timeinfo;
  if(povezan)br_pokusaja = 0U;
  while (!getLocalTime(&timeinfo) && br_pokusaja < BR_POKUSAJA_POVEZIVANJA && povezan) {    // slanje zahteva ka internet serveru koji daje date/time
    br_pokusaja += 1U;
  }
  if(br_pokusaja < BR_POKUSAJA_POVEZIVANJA){
    status_sistema = STATUS_OK;
  }
  br_pokusaja = 0U;

  v.sek = timeinfo.tm_sec;
  v.min = timeinfo.tm_min;
  v.sat = timeinfo.tm_hour;
  v.dan = timeinfo.tm_mday;
  v.dan_u_nedelji = timeinfo.tm_wday;
  v.mesec = timeinfo.tm_mon + 1U;
  v.god = timeinfo.tm_year + 1900U;

  Serial.println(String(v.dan) + "." + String(v.mesec) + "." + String(v.god));
  Serial.println(String(v.sat) + ":" + String(v.min) + ":" + String(v.sek));

  v.min = 29;
  v.sat = 6;
  v.sek = 50;
  WiFi.disconnect(true);

}

void loop() {
  // put your main code here, to run repeatedly:

  vremenski_pomak();
  if(v.sat == 6U && v.min == 30U && !sinhronizovan){
    sinhronizacija();
  }
  else if(v.sat == 6U && v.min == 31U && sinhronizovan){
    sinhronizovan = false;
  }
  
  if(prekid_tajmera){
    Serial.println(String(v.dan) + "." + String(v.mesec) + "." + String(v.god));
    Serial.println(String(v.sat) + ":" + String(v.min) + ":" + String(v.sek));
    if(br_prekida_tajmera > 5){
      digitalWrite(PIN_KAZALJKA1, LOW);
      digitalWrite(PIN_KAZALJKA2, LOW);
      br_prekida_tajmera = 0U;
    }
    prekid_tajmera = false;
  }
  //Serial.println(br_debouncing_taster1);
  if(prekid_tastera1 && br_debouncing_taster1 > DEBOUNCING_INTERVAL){
    Serial.println("pritisak tastera 1");
    posalji_impuls();
    prekid_tastera1 = false;
    br_debouncing_taster1 = 0;
  }

  br_debouncing_taster1++;
  if(br_debouncing_taster1 > 28000) br_debouncing_taster1 = 27800;

}

void vremenski_pomak(){
  if(v.sek >= 60U){
    v.sek = 0U;
    v.min += 1U;
    posalji_impuls();
    if(v.min >= 60U){
      v.min = 0U;
      v.sat += 1U;
      if(v.sat >= 24U){
        v.sat = 0U;
        v.dan += 1U;
        if((v.mesec == 1U || v.mesec == 3U || v.mesec == 5U || v.mesec == 7U || v.mesec == 8U || v.mesec == 10U || v.mesec == 12U) && v.dan > 31U){
          v.dan = 1U;
          v.mesec += 1U;
          if(v.mesec > 12U){
            v.mesec = 0U;
            v.god += 1U;
            prestupna_godina = false;
          }
        }
        if((v.mesec == 4U || v.mesec == 6U || v.mesec == 9U || v.mesec == 11U) && v.dan > 30U){
          v.dan = 1U;
          v.mesec += 1U;
        }
        if(v.mesec == 2U && v.dan > 28U){
          if ((v.god % 4 == 0 && v.god % 100 != 0) || (v.god % 400 == 0)) {
            prestupna_godina = true;
            if(v.dan > 29U){
              v.dan = 1U;
              v.mesec += 1U;
            }
          }
        }
        if(v.mesec == 2U && v.dan > 28U && !prestupna_godina){
          v.dan = 1U;
          v.mesec += 1U;
          
        }
      }
    }

    

  }
}

void sinhronizacija(){
  WiFi.begin(ssid_STA, password_STA); // inicijalizacija WiFi
  uint8_t br_pokusaja = 0U;
  while(WiFi.status() != WL_CONNECTED && br_pokusaja < BR_POKUSAJA_POVEZIVANJA){   // povezivanje na WiFi mrezu (koja ima internet)
    Serial.write(".");
    br_pokusaja += 1U;
    delay(10U);
  }
  if(br_pokusaja < BR_POKUSAJA_POVEZIVANJA){
    povezan = true;
    Serial.println(WiFi.localIP());
  }

  
  struct tm timeinfo;
  if(povezan)br_pokusaja = 0U;
  while (!getLocalTime(&timeinfo) && br_pokusaja < BR_POKUSAJA_POVEZIVANJA && povezan) {    // slanje zahteva ka internet serveru koji daje date/time
    br_pokusaja += 1U;
    delay(10U);
  }
  if(br_pokusaja >= BR_POKUSAJA_POVEZIVANJA){
    status_sistema = STATUS_WARNING_TIME_NOT_CORRECTED;
  }
  else{
    v.sek = timeinfo.tm_sec;      // nakon dobijenih info, osvezavanje vremena (korekcija)
    v.min = timeinfo.tm_min;
    v.sat = timeinfo.tm_hour;
    v.dan = timeinfo.tm_mday;
    v.dan_u_nedelji = timeinfo.tm_wday;
    v.mesec = timeinfo.tm_mon + 1U;
    v.god = timeinfo.tm_year + 1900U;
    sinhronizovan = true;
    status_sistema = STATUS_OK;
  }

 

  Serial.println(String(v.god) + " " + String(v.mesec) + " " + String(v.dan));
  Serial.print(String(v.sat) + " " + String(v.min) + " " + String(v.sek));
  WiFi.disconnect(true);
  
}

void posalji_impuls(){
  if(pin1_na_low){
    digitalWrite(PIN_KAZALJKA1, LOW);
    digitalWrite(PIN_KAZALJKA2, HIGH);
    pin1_na_low = false;
  }
  else{
    digitalWrite(PIN_KAZALJKA1, HIGH);
    digitalWrite(PIN_KAZALJKA2, LOW);
    pin1_na_low = true;
  }
}

void IRAM_ATTR onTimer0(){
  portENTER_CRITICAL_ISR(&timerMux0);
  br_sekundi += 1;
  v.sek += 1;
  prekid_tajmera = true;
  br_prekida_tajmera += 1;
  portEXIT_CRITICAL_ISR(&timerMux0);
}

void IRAM_ATTR handleExternalTaster1(){
  portENTER_CRITICAL_ISR(&muxTaster1);
  
  prekid_tastera1 = true;
  br_debouncing_taster1 = 0;
  
  portEXIT_CRITICAL_ISR(&muxTaster1);
}

void IRAM_ATTR handleExternalTaster2(){
  portENTER_CRITICAL_ISR(&muxTaster2);
  portEXIT_CRITICAL_ISR(&muxTaster2);
}
