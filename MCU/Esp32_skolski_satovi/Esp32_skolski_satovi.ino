

/*
  Esp32 mikrokontroler pokrece skolske satove i pali zvono u odredjenim trenucima. Komunikacija sa Android uredjajem preko WiFi za dobijanje sablona po kojem se zvoni i jos neke info. Postoje i tasteri za podesavanje satova.
  Za visoku preciznost merenja vremena koriste se 2 pristupa: povezivanje na lokalnu internet mrezu zarad dobijanja date/time informacije. Takodje vreme se meri unutrasnjim tajmerima. 
  Info sa interneta se moze koristiti za korekciju i sinhronizaciju.
  Mehanizam kazaljke radi tako sto se promene stanja 2 pina (toggle). Jedanput pin1 promena na HIGH i pin2 promena na LOW, pa onda obrnuto kad bude sledeca minuta. U medjuvremenu oba pina su ugasena zbog ustede.
  Postoji i 1 taster za zujalicu i 1 taster za pomeranje kazaljke. Rasporedi zvona se dobijaju od Androida. Ako je ukljuceno, automatski se ukljucuje zvono u odredjenin trenucima.

  Softver: Nemanja M.
  Hardver: Dragan M.
  Decembar 2024.

*/

#include <WiFi.h>
#include <string>
#include <time.h>
#include <vector>
#include <iterator>
#include <EEPROM.h>
#include "esp_system.h"
#include "driver/rtc_io.h"
#include "soc/rtc.h"
#include "esp_task_wdt.h"



using namespace std;

// izlazni
#define PIN_KAZALJKA1 33  // 2 pina za teranje kazaljki, naizmenicno menjanje njihovih stanja za pokretanje kazaljke
#define PIN_KAZALJKA2 12
#define PIN_ZVONO 13    

// ulazni
#define PIN_TASTER1 4   // kazaljka napred
#define PIN_TASTER2 15   // zvono
 

#define MAX_DUZINA 25
#define MAX_DUZINA_SSID 32
#define MAX_DUZINA_PASSWORD 64

#define BR_POKUSAJA_POVEZIVANJA 20
#define DEBOUNCING_INTERVAL1 10000    // preciznije saltanje kazaljke, sporije, u blizini odredjene minute
#define DEBOUNCING_INTERVAL2 1000   // brze saltanje kazaljke, za brze dostizanje odredjenog sata i minute
#define TIMEOUT_POVEZIVANJE 70U   // interval povezivanja, ako premasi, restart
#define BROJ_SINHRONIZACIJA_U_SATU 23

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

const char* ssid_AP = "SAT_ESP32";     
const char* password_AP = "jswomvjknmc";
//const char* password_AP = "tisovac2";

char ssid_STA[MAX_DUZINA_SSID] = "default";
char password_STA[MAX_DUZINA_PASSWORD] = "password";

char NTP_server[MAX_DUZINA] = "pool.ntp.org";   // internet server za primanje info o vremenu
const long gmtPomak = 3600U;       // Pomak u sekundama od GMT (za CET 3600)
const int letnje_aktivno = 3600U;   // Letnje računanje vremena (ako je aktivno)

WiFiClient client;
WiFiServer server(80);
String str_za_wifi = "";

Vreme v;
volatile bool sinhronizovan = false;
volatile bool prekid_tajmera = false;   // menja se u ISR
bool prekid_tastera1 = false;
bool prekid_tastera2 = false;
bool prestupna_godina = false;
volatile uint8_t status_sistema = STATUS_ERR_UNKNOWN_TIME;   // varijabla koja govori u kom stanju se nalazi sistem
bool povezan = false;
bool pin1_na_low = true; // info da pamti koji pin treba da se ugasi a koji da se upali kad dodje vreme da se posalje signal kazaljki sata. U medjuvremenu su ugasena oba pina zbog ustede. Signalizacija traje x ms.
volatile uint8_t br_prekida_tajmera = 0U;
volatile uint8_t rucni_watchdog = 0U;
uint16_t br_debouncing_taster1 = 0U;
uint16_t br_debouncing_taster2 = 0U;

uint8_t sinhronizacija_sati = 6;    // vreme sinhronizacije sa sistemskim vremenom, preko interneta
uint8_t sinhronizacija_min = 30;
bool ukljuceno_automatsko_zvono = true;
volatile bool u_toku_automatsko_podesavanje_sata = false;
volatile uint8_t br_min_auto_podesavanje_sata = 0U;    // broj minuta protekao od kako je pocelo automatsko podesavanje sata
int16_t poteraj_kazaljku_automatski_br_min = 0;   // broj minuta za koji treba poterati kazaljku kod automatskog namestanja sata
volatile uint8_t br_prekida_tajmera_auto_kazaljka = 0U; // tokom automatskog namestanja sata, kazaljka bi trebalo da se pomera npr jednom u sekundi, bez prevelikog opterecenja
volatile bool sinhronizacija_u_toku = false;
uint8_t br_sek_taster_zvono_pritisnut = 0U; // taster mora biti pritisnut neko vreme da se aktivira zvono
bool pritisnut_taster_zvono = false;      // ako je registrovano LOW na pinu za taster zvona
volatile uint8_t trajanje_sinhronizacije = 0U; // treba nam info koliko je sinhronizacija trajala, ako propadne, da se precizno nadoknadi vreme
volatile int br = 0;
bool dnevni_rezim = true; // ucestanost sinhronizacija je razlicita po danu i po noci

vector<uint8_t> raspored_zvona_sati;    // spisak svih sati kad treba zvoniti
vector<uint8_t> raspored_zvona_min;   // spisak svih minuta kad treba zvoniti (uslov da zvoni jeste da se poklapa i sat i minuta iz oba vektora sa trenutnim vremenom)


uint8_t vreme_sync[] = {3U, 6U, 9U, 12U, 14U, 17U, 19U, 22U, 24U, 27U, 29U, 32U, 34U, 37U, 39U, 42U, 44U, 47U, 49U, 52U, 54U, 57U, 59U};    // vreme sinhronizacije u toku jednog sata, svakih 5min
uint8_t vreme_restart[] = {12,23,34,46,52}; // test

struct tm rtcTime;

hw_timer_t* timer0 = NULL; 
portMUX_TYPE timerMux0 = portMUX_INITIALIZER_UNLOCKED;

hw_timer_t* timer1 = NULL; 
portMUX_TYPE timerMux1 = portMUX_INITIALIZER_UNLOCKED;

hw_timer_t* timer2 = NULL;  // timer2 kao tajmer da zastiti od zaglavljivanja u setup-u, daje se odredjen interval da se zavrsi setup i ako se ne zavrsi do tad, restart mikrokontrolera
portMUX_TYPE timerMux2 = portMUX_INITIALIZER_UNLOCKED;

portMUX_TYPE muxTaster1 = portMUX_INITIALIZER_UNLOCKED;                  //za external interrupt
portMUX_TYPE muxTaster2 = portMUX_INITIALIZER_UNLOCKED; 

void IRAM_ATTR onTimer0();    // prekidne rutine unutrasnjeg tajmera i spoljasnjih tastera
void IRAM_ATTR onTimer1();    // prekidne rutine unutrasnjeg tajmera i spoljasnjih tastera
void IRAM_ATTR handleExternalTaster1();
void IRAM_ATTR handleExternalTaster2();
void vremenski_pomak();     // na osnovnu vremenskih info (sek, min, sat...) pomera ostale vrednosti u skladu sa pravilima
void sinhronizacija();   // povezivanje na internet server i sinhronizacija sa vremenskom zonom, korekcija akumulirane greske, jednom dnevno
void posalji_impuls_kazaljka();  // postavlja stanja pinova tako da pokrene mehanizam kazaljke sata
void upali_zvono();       // postavlja stanje pina tako da upali zvono
void posalji_stanje_WiFi();      // salje preko WiFi stanje Android uredjaju
void automatsko_podesavanje_sata(uint8_t sati, uint8_t min);    // automatsko podesavanje sata, prima vreme koje je na satu (sat je stao) i automatski ga podesava na trenutno
void obrada_stringa(String str);  // parsiranje primljenog stringa preko WiFi
void provera_string_sati(String str, uint8_t* sati, uint8_t* min);    // parsiranje stringa
void provera_string_ruter(String str, char ssid[], char password[]);
void upisiPodatak(int adresa, int vrednost, uint8_t duzina);    // upisivanje i citanje iz memorije
template<typename T> T ocitajPodatak(int adresa);
void ucitaj_iz_memorije();    // ucitavanje svih podataka iz memorije
int duzina_char_stringa(char str[]);
void reset();  // resetovanje sistema

void setup() {

  delay(1000U);
  setCpuFrequencyMhz(80);   // najniza brzina radnog takta
  
/*  esp_reset_reason_t razlog_reseta = esp_reset_reason();
  rtc_clk_slow_freq_set(RTC_SLOW_FREQ_RTC);
  if(razlog_reseta != ESP_RST_WDT && razlog_reseta != ESP_RST_SW){
    rtc_clk_32k_enable(true);
    delay(500U);
    rtc_clk_slow_freq_set(RTC_SLOW_FREQ_32K_XTAL);
  }
*/
  Serial.begin(115200);
  EEPROM.begin(512);
 /* rtc_clk_32k_enable(true);
  rtc_clk_slow_freq_set(RTC_SLOW_FREQ_32K_XTAL);*/

  timer0 = timerBegin(0, 80U, true);      // inicijalizacija tajmera koji meri vreme, za postavljanje stanja na pinove
  timerAttachInterrupt(timer0, &onTimer0, true);
  timerAlarmWrite(timer0, 1000000, true);     // korak od 1sec
  timerAlarmEnable(timer0); // omogucavanje prekida

  delay(150);

  timer1 = timerBegin(1, 80U, true);      // inicijalizacija tajmera koji sluzi tokom automatskog namestanja sata
  timerAttachInterrupt(timer1, &onTimer1, true);
  timerAlarmWrite(timer1, 250000, true);     // korak od 250ms
  //timerAlarmEnable(timer1);

  pinMode(PIN_TASTER1, INPUT_PULLUP);    // tasteri na ulaznim pinovima
  pinMode(PIN_TASTER2, INPUT_PULLUP);    //

  
  pinMode(PIN_KAZALJKA1, OUTPUT);   // 2 pina za upravljanje kazaljkama
  digitalWrite(PIN_KAZALJKA1, LOW);

  pinMode(PIN_KAZALJKA2, OUTPUT);   
  digitalWrite(PIN_KAZALJKA2, HIGH);

  pinMode(PIN_ZVONO, OUTPUT);   
  digitalWrite(PIN_ZVONO, HIGH);    // HIGH je iskljuceno zvono, LOW je ukljuceno

  v.sek = 0U;
  v.min = 0U;
  v.sat = 0U;
  v.dan = 1U;
  v.dan_u_nedelji = 1U;    // 0 - nedelja, 1 - pon, ..., 6 - subota
  v.mesec = 1U;
  v.god = 2025U;

  ucitaj_iz_memorije();

  delay(150U);
  sinhronizacija();
  
  if (WiFi.softAP(ssid_AP, password_AP)) {  // inicijalizacija sopstvene WiFi mreze
      Serial.println("kreirana mreza");
  }  
  server.begin();     // pokretanje mreze

  esp_task_wdt_init(TIMEOUT_POVEZIVANJE, true);   // watchdog timer
  esp_task_wdt_add(NULL);   // dodavanje ovog task-a
/*
  rtc_clk_slow_freq_set(RTC_SLOW_FREQ_32K_XTAL);
  rtc_clk_32k_enable(true);
  */
  delay(500U);

}

void loop() {

 // esp_task_wdt_reset();   // sprecavanje reseta ako se izvrsava kod normalno

  if(!client)client = server.available();   // provera da li je neko pokusao da se poveze na mrezu od ESP32, ako jeste vraca objekat client
  else{
    Serial.println("klijent povezan");
    if(client.connected()){   // provera da li je veza aktivna i validna
      while (client.available()) {     //ako postoje podaci za citanje sa WiFi, available() vraca broj bajtova koji su primljeni od klijenta
        String str = client.readStringUntil('.');
        Serial.println(str);
        obrada_stringa(str);
        delay(2);
        String ack = "t";
        client.println(ack);    // obavestavanje posiljaoca da je poruka uspesno primljena
        delay(2);
        /*char c = client.read();  // Čitamo bajt po bajt
        Serial.write(c);  // Šaljemo bajtove na Serial Monitor*/
      }
    }
    else{
      client.stop();      // ako je doslo do prekida veze,da se sacuva ovaj objekat, da ne bude u zaglavljenom stanju
      client = WiFiClient();    // reset klijenta
      Serial.println("Klijent prekinuo vezu");
    }
  }

  vremenski_pomak();
  

  
  if(prekid_tajmera){   // prosla 1sec
   // Serial.println(String(v.dan) + "." + String(v.mesec) + "." + String(v.god));
   // Serial.println(String(v.sat) + ":" + String(v.min) + ":" + String(v.sek));
    if(br_prekida_tajmera > 4){
      digitalWrite(PIN_KAZALJKA1, LOW);   // nakon kraceg intervala od pocetka minute, oba pina staviti na LOW zbog ustede
      digitalWrite(PIN_KAZALJKA2, LOW);
      digitalWrite(PIN_ZVONO, HIGH);    // ugasiti zvono
    }

    if(!sinhronizacija_u_toku){
      v.sek += 1;
    }
   /* else if(sinhronizacija_u_toku){
      rucni_watchdog += 1U;
      if(rucni_watchdog > TIMEOUT_POVEZIVANJE){     // 30sec za povezivanje, ako premasi
        WiFi.mode(WIFI_OFF);
        delay(100U);
        sinhronizacija();
      }
    }
  */
    if(pritisnut_taster_zvono){
      br_sek_taster_zvono_pritisnut += 1;
    }

    prekid_tajmera = false;
  }

  if(br_prekida_tajmera_auto_kazaljka > 1){
      automatsko_podesavanje_sata();
      br_prekida_tajmera_auto_kazaljka = 0U;
  }

  //Serial.println(br_debouncing_taster1);
  if(digitalRead(PIN_TASTER1) == LOW && br_debouncing_taster1 > DEBOUNCING_INTERVAL1){      // polling, ispitivanje da li su tasteri pritisnuti
    Serial.println("pritisak tastera 1");                                               // metoda prekida je onemogucena zbog suma u napajanju koje dolazi sa 220V, i izaziva prekide kad ne treba
    posalji_impuls_kazaljka();
    br_debouncing_taster1 = 0;
  }

  if(digitalRead(PIN_TASTER2) == LOW/* && br_debouncing_taster2 > DEBOUNCING_INTERVAL1*/){      // polling, ispitivanje da li su tasteri pritisnuti
    Serial.println("pritisak tastera 2");                                               // metoda prekida je onemogucena zbog suma u napajanju koje dolazi sa 220V, i izaziva prekide kad ne treba
    pritisnut_taster_zvono = true;
    if(br_sek_taster_zvono_pritisnut >= 3U){   // duzi pritisak
      upali_zvono();
      br_prekida_tajmera = 0U;
    }
   // br_debouncing_taster2 = 0;
  }
  else if(digitalRead(PIN_TASTER2) == HIGH){
    br_sek_taster_zvono_pritisnut = 0;
    pritisnut_taster_zvono = false;
  }

 /* if(digitalRead(PIN_TASTER2) == LOW && br_debouncing_taster2 > DEBOUNCING_INTERVAL2){
    Serial.println("pritisak tastera 2");
    brojac_prekida += 1;
    posalji_impuls();
    br_debouncing_taster2 = 0;
  }*/

  br_debouncing_taster1++;
  if(br_debouncing_taster1 > 28000) br_debouncing_taster1 = 27800;
  if(br_prekida_tajmera > 250) br_prekida_tajmera = 248;
  if(br_prekida_tajmera_auto_kazaljka > 250) br_prekida_tajmera_auto_kazaljka = 248;

  br_debouncing_taster2++;
  if(br_debouncing_taster2 > 28000) br_debouncing_taster2 = 27800;

}

void vremenski_pomak(){
  
  if(v.sek >= 60U/*rtcTime.tm_min != v.min*/){
    
    //sinhronizacija();
    v.sek = 0U;
    v.min += 1U;

    rtc_clk_32k_enable(true);
    rtc_clk_slow_freq_set(RTC_SLOW_FREQ_32K_XTAL);

    if(!u_toku_automatsko_podesavanje_sata){
      posalji_impuls_kazaljka();
    }
    br_prekida_tajmera = 0U;

    if(u_toku_automatsko_podesavanje_sata && poteraj_kazaljku_automatski_br_min > 0){
      br_min_auto_podesavanje_sata += 1;
    }
   
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

    Serial.println(String(v.dan) + "." + String(v.mesec) + "." + String(v.god));
    Serial.println(String(v.sat) + ":" + String(v.min) + ":" + String(v.sek));  

    if(v.sat >= 5 && v.sat <= 21){   //   5-22h
      dnevni_rezim = true;
      for(uint8_t i = 0; i < BROJ_SINHRONIZACIJA_U_SATU; i++){
        if((v.min == vreme_sync[i])){
          sinhronizacija();
        }
      }
    }
    else{
      WiFi.mode(WIFI_OFF);      
      dnevni_rezim = false;
      if(v.min == 2 || v.min == 32){
        sinhronizacija();
      }
    }
/*
    if(v.sat == 18 && v.min == 33){
      delay(2000U);
      reset();
    }
*/
    vector<uint8_t>::iterator it1;
    vector<uint8_t>::iterator it2;
    int i = 0;
    int j = 0;
    for(it1 = raspored_zvona_sati.begin(); it1 != raspored_zvona_sati.end(); it1++){    // prolazak kroz spisak svih sati u kojima treba zvoniti
      if(*it1 == v.sat){        // ako je trenutni sat jednak sa nekim u spisku
        j = 0;
        for(it2 = raspored_zvona_min.begin(); it2 != raspored_zvona_min.end(); it2++){    // prolazak kroz spisak svih minuta u kojima treba zvoniti
          if(i == j && *it2 == v.min && ukljuceno_automatsko_zvono){      // vazni su indeksi, jer su jednako poredjani sati i minute u odvojenim spiskovima kad treba zvoniti
            upali_zvono();
          }
          j += 1;
        }
      }
      i += 1;
    }

  }

  

}

void sinhronizacija(){
  sinhronizacija_u_toku = true;
  WiFi.mode(WIFI_STA);      // pri povezivanju na ruter, prelazi u rezim WiFi stanice
  delay(100U);
  WiFi.begin(ssid_STA, password_STA); // inicijalizacija WiFi, pre pocetka povezivanja
  uint8_t br_pokusaja = 0U;
  while(WiFi.status() != WL_CONNECTED && br_pokusaja < BR_POKUSAJA_POVEZIVANJA){   // povezivanje na WiFi mrezu (koja ima internet)
    Serial.write(".");
    br_pokusaja += 1U;
    delay(1000U);
  }
  
  if(br_pokusaja < BR_POKUSAJA_POVEZIVANJA){
    povezan = true;
    Serial.println(WiFi.localIP());
  }
  else{
    povezan = false;
    if(status_sistema == STATUS_OK || status_sistema == STATUS_WARNING_TIME_NOT_CORRECTED){
      status_sistema = STATUS_WARNING_TIME_NOT_CORRECTED;
    }
  }

  if(povezan){
    configTime(gmtPomak, letnje_aktivno, NTP_server); // slanje zahteva ka internet serveru koji daje date/time, upis u RTC
    Serial.println("Povezan na ruter!");
    delay(2000U);
  }
  struct tm timeinfo;
  getLocalTime(&timeinfo);    // citanje iz RTC

  if(/*br_pokusaja >= BR_POKUSAJA_POVEZIVANJA*/ !povezan){   // neuspesno
    if(status_sistema == STATUS_OK || status_sistema == STATUS_WARNING_TIME_NOT_CORRECTED){
      status_sistema = STATUS_WARNING_TIME_NOT_CORRECTED;
    }
    v.sek = timeinfo.tm_sec;      // propala sinhronizacija, uzimanje vremena iz RTC
    v.min = timeinfo.tm_min;
    v.sat = timeinfo.tm_hour;
  }
  else{     // uspesno
    v.sek = 0U;
    v.min = 0U;
    v.sat = 0U;
    v.sek = timeinfo.tm_sec;      // nakon dobijenih info, osvezavanje vremena (korekcija)
    v.min = timeinfo.tm_min;
    v.sat = timeinfo.tm_hour;
    v.dan = timeinfo.tm_mday;
    v.dan_u_nedelji = timeinfo.tm_wday;
    v.mesec = timeinfo.tm_mon + 1U;
    v.god = timeinfo.tm_year + 1900U;
    sinhronizovan = true;
    status_sistema = STATUS_OK;
    Serial.println(String(v.dan) + "." + String(v.mesec) + "." + String(v.god));
    Serial.println(String(v.sat) + ":" + String(v.min) + ":" + String(v.sek));
  }
  WiFi.disconnect(true, true);
  delay(100U);
  sinhronizacija_u_toku = false;
  rucni_watchdog = 0U;
  trajanje_sinhronizacije = 0U;
  WiFi.mode(WIFI_AP);   // kad se zavrsi sinhronizacija, povratak u WiFi rezim AP
}

void reset(){
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  //rtc_clk_slow_freq_set(RTC_SLOW_FREQ_RTC);
  delay(200U);
  int8_t br = 0;
  while(br < (TIMEOUT_POVEZIVANJE + 5)){br+=1;}   // do aktivacije watchdog
}

void posalji_impuls_kazaljka(){
  if(pin1_na_low){      // naizmenicno menjanje stanja pinova, za pokretanje kazaljke
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

void upali_zvono(){
  digitalWrite(PIN_ZVONO, LOW);
  Serial.println("Zvonim: " + String(v.sat) + ":" + String(v.min));
}
void posalji_stanje_WiFi(){
    if(status_sistema == STATUS_OK){
      client.println("0");
    }
    if(status_sistema == STATUS_ERR_UNKNOWN_TIME){
      client.println("1");
    }
    if(status_sistema == STATUS_WARNING_TIME_NOT_CORRECTED){
      client.println("2");
    }
}

void automatsko_podesavanje_sata(){
  if(u_toku_automatsko_podesavanje_sata && poteraj_kazaljku_automatski_br_min > 0) {
      posalji_impuls_kazaljka();
      poteraj_kazaljku_automatski_br_min -= 1;
      Serial.println(poteraj_kazaljku_automatski_br_min);
  }
  if(u_toku_automatsko_podesavanje_sata && poteraj_kazaljku_automatski_br_min == 0){
    if(br_min_auto_podesavanje_sata > 0){
      posalji_impuls_kazaljka();
      br_min_auto_podesavanje_sata -= 1;
    //  Serial.println(br_min_auto_podesavanje_sata);
    }
    else if(br_min_auto_podesavanje_sata == 0){
      u_toku_automatsko_podesavanje_sata = false;
      timerAlarmDisable(timer1);
   //   Serial.println(br_min_auto_podesavanje_sata);
    }
    
  }
}

void provera_string_sati(String str, uint8_t* sati, uint8_t* min){
    String sati_str = "";
    String min_str = "";
    int indeks_delimitera = 0;
    indeks_delimitera = str.indexOf(':');
    for(int i = 1; i < indeks_delimitera; i++){
      sati_str += str[i];
    }
    for(int i = indeks_delimitera + 1; i < str.length(); i++){
      min_str += str[i];
    }

    *sati = (uint8_t)sati_str.toInt();
    *min = (uint8_t)min_str.toInt();
}
void provera_string_ruter(String str, char ssid[], char password[]){
    
    int indeks_delimitera = 0;
    indeks_delimitera = str.indexOf('_');
    int j = 0;

    for(int i = 1; i < indeks_delimitera; i++){
      ssid[j] = str[i];
      j += 1;
    }
    ssid[j] = '\0';
    j = 0;
    for(int i = indeks_delimitera + 1; i < str.length(); i++){
      password[j] = str[i];
      j += 1;
    }
    password[j] = '\0';
    Serial.println(ssid);
    Serial.println(password);
}

void obrada_stringa(String str){      // obradjivanje svih ulaznih poruka

  if(str[0] == 'a'){        // pali zvono
    br_prekida_tajmera = 0U;
    upali_zvono();
  }
  else if(str[0] == 'b'){         // vrati status sistema
    posalji_stanje_WiFi();
  }
  else if(str[0] == 'c'){       // pomeri kazaljku za +1
    posalji_impuls_kazaljka();
  }
  else if(str[0] == 'd'){       // automatsko namestanje sata, prima se info na koliko sat stoji 
    uint8_t blokada_sati = 0;
    uint8_t blokada_min = 0;
    provera_string_sati(str, &blokada_sati, &blokada_min);
    if(blokada_sati > 12){
      blokada_sati -= 12;
    }

    int16_t vreme_zastoja_min = (blokada_sati * 60U) + blokada_min;
    
    uint8_t trenutno_vreme_sat = v.sat;
    if(trenutno_vreme_sat > 12){
      trenutno_vreme_sat -= 12;
    }
    int16_t trenutno_vreme_min = (trenutno_vreme_sat * 60U) + v.min;
    Serial.println("trenutno vreme min " + String(trenutno_vreme_min));
    Serial.println("zastoj vreme min " + String(vreme_zastoja_min));
    int16_t razlika_min = trenutno_vreme_min - vreme_zastoja_min;
    Serial.println("razlika " + String(razlika_min));
    if(razlika_min < 0) {
      razlika_min *= (-1);
    }
    Serial.println("razlika " + String(razlika_min));
    if(trenutno_vreme_min > vreme_zastoja_min){
        poteraj_kazaljku_automatski_br_min = razlika_min;
    }
    else{
      poteraj_kazaljku_automatski_br_min = 720 - razlika_min;
    }
    Serial.println("Teraj " + String(poteraj_kazaljku_automatski_br_min));

    timerAlarmEnable(timer1);
    u_toku_automatsko_podesavanje_sata = true;
  }
  else if(str[0] == 'e'){       // vreme povezivanja na internet ruter za dobijane preciznije info o trenutnom vremenu
    uint8_t sati = 0;
    uint8_t min = 0;
    provera_string_sati(str, &sati, &min);
    sinhronizacija_sati = sati;
    sinhronizacija_min = min;
    upisiPodatak(73, sinhronizacija_sati, 1);
    upisiPodatak(74, sinhronizacija_min, 1);
  }
  else if(str[0] == 'f'){     // SSID i Lozinka internet rutera
    provera_string_ruter(str, ssid_STA, password_STA);
    int duzina_ssid = duzina_char_stringa(ssid_STA);
    upisiPodatak(75, (uint8_t)duzina_ssid, 1);
    int j = 76;
    for(int i = 0; i < duzina_ssid; i++){
      upisiPodatak(j, ssid_STA[i], 1);
      j += 1;
    }

    int duzina_password = duzina_char_stringa(password_STA);
    upisiPodatak(109, (uint8_t)duzina_password, 1);
    j = 110;
    for(int i = 0; i < duzina_password; i++){
      upisiPodatak(j, password_STA[i], 1);
      j += 1;
    }
  }
  else if(str[0] == 'g'){     // ukljuci/iskljuci automatsko zvono
    if(str[1] == '0'){
      ukljuceno_automatsko_zvono = false;
    }
    else if(str[1] == '1'){
      ukljuceno_automatsko_zvono = true;
    }
    upisiPodatak(72, ukljuceno_automatsko_zvono, 1);
  }
  else if(str[0] == 'h'){         // raspored zvona u toku dana
      char delimiter_zvona = '_';
      char delimiter_sat_min = ':';

      raspored_zvona_sati.clear();
      raspored_zvona_min.clear();


      int i = 1;
      bool odradjen_sat = false;
      String sat_str = "";
      String min_str = "";
      uint8_t sat = 0;
      uint8_t min = 0;
      while(i < str.length()){
        int j = 0;
        
        while(str[i] != ':' && !odradjen_sat){
          sat_str += str[i];
          i += 1;
        }
        i += 1;
        odradjen_sat = true;
        sat = (uint8_t)sat_str.toInt();
        raspored_zvona_sati.push_back(sat);
        while(str[i] != '_' && odradjen_sat){
          min_str += str[i];
          i += 1;
        }

        odradjen_sat = false;
        
        min = (uint8_t)min_str.toInt();
        raspored_zvona_min.push_back(min);
        sat_str = "";
        min_str = "";
        i += 1;
      }
      vector<uint8_t>::iterator it;
      for(it = raspored_zvona_sati.begin(); it != raspored_zvona_sati.end(); it++){
        Serial.println(*it);
      }
      for(it = raspored_zvona_min.begin(); it != raspored_zvona_min.end(); it++){
        Serial.println(*it);
      }

      upisiPodatak(0, (uint8_t)raspored_zvona_sati.size(), 1);
      int j = 1;
      for(it = raspored_zvona_sati.begin(); it != raspored_zvona_sati.end(); it++){
        upisiPodatak(j, *it, 1);
        j += 1;
      }
      upisiPodatak(36, (uint8_t)raspored_zvona_min.size(), 1);
      j = 37;
      for(it = raspored_zvona_min.begin(); it != raspored_zvona_min.end(); it++){
        upisiPodatak(j, *it, 1);
        j += 1;
      }
      
    }
    else if(str[0] == 'i'){     // test
      String str = String(v.sat) + ":" + String(v.min) + ".";
      client.println(str);
    }
  }



void ucitaj_iz_memorije(){
    // ucitavanje iz memorije po mapi, koja je rucno sastavljena
    Serial.println("ucitavanje");
    uint8_t velicina_vektora = ocitajPodatak<uint8_t>(0);
    for(int i = 1; i < 1 + velicina_vektora; i++){
      uint8_t podatak = ocitajPodatak<uint8_t>(i);
      raspored_zvona_sati.push_back(podatak);
    }
    for(int i = 37; i < 37 + velicina_vektora; i++){
      uint8_t podatak = ocitajPodatak<uint8_t>(i);
      raspored_zvona_min.push_back(podatak);
    }

    ukljuceno_automatsko_zvono = ocitajPodatak<bool>(72);
    sinhronizacija_sati = ocitajPodatak<uint8_t>(73);
    sinhronizacija_min = ocitajPodatak<uint8_t>(74);

     uint8_t velicina_ssid = ocitajPodatak<uint8_t>(75);
     int j = 76;
     int i = 0;
     for(i = 0; i < velicina_ssid; i++){
      char podatak = ocitajPodatak<char>(j);
      ssid_STA[i] = podatak;
      j += 1;
     }
     ssid_STA[i] = '\0';
      
    uint8_t velicina_password = ocitajPodatak<uint8_t>(109);
     j = 110;
     for(i = 0; i < velicina_password; i++){
      char podatak = ocitajPodatak<char>(j);
      password_STA[i] = podatak;
      j += 1;
     }
     password_STA[i] = '\0';


    vector<uint8_t>::iterator it;
    for(it = raspored_zvona_sati.begin(); it != raspored_zvona_sati.end(); it++){
      Serial.println(*it);
    }
    for(it = raspored_zvona_min.begin(); it != raspored_zvona_min.end(); it++){
      Serial.println(*it);
    }
    Serial.println(ukljuceno_automatsko_zvono);
    Serial.println(sinhronizacija_sati);
    Serial.println(sinhronizacija_min);

    for(int i = 0; i < velicina_ssid; i++){
      Serial.println(ssid_STA[i]);
    }

    for(int i = 0; i < velicina_password; i++){
      Serial.println(password_STA[i]);
    }
      
  }







void upisiPodatak(int adresa, int vrednost, uint8_t duzina){
    if(duzina == 1){ //1B
        EEPROM.write(adresa, vrednost);
    }
    else if(duzina == 4 || duzina == 2 || duzina == 8){   //2B+
        EEPROM.put(adresa, vrednost);
    }
    EEPROM.commit();
}  

template<typename T> 
T ocitajPodatak(int adresa){       // tip funkcije moze biti int i uint8_t (4B ili 1B) da se izbegne dupliranje koda
    T vrednost;
    EEPROM.get(adresa, vrednost);
    return vrednost;
}

int duzina_char_stringa(char str[]){
  int i = 0;
  while(str[i] != '\0'){
    i += 1;
  }
  return i;
}
  


void IRAM_ATTR onTimer0(){
  //portENTER_CRITICAL_ISR(&timerMux0);
  prekid_tajmera = true;
  if(sinhronizacija_u_toku){
    trajanje_sinhronizacije += 1U;
  }
  br_prekida_tajmera += 1U;
  //portEXIT_CRITICAL_ISR(&timerMux0);
}

void IRAM_ATTR onTimer1(){
  portENTER_CRITICAL_ISR(&timerMux1);
  br_prekida_tajmera_auto_kazaljka += 1U;
  portEXIT_CRITICAL_ISR(&timerMux1);
}




/*
  MAPA MEMORIJE:
  0 - velicina vektora sati
  1-35 - vektor sati
  36 - velicina vektora min (ista)
  37-71 - vektor min
  72 - ukljuceno_zvono
  73 - sync_sati
  74 - sync_min
  75 - velicina ssid stringa
  76-108 - ssid
  109 - velicina password stringa
  110-172 - password
*/

