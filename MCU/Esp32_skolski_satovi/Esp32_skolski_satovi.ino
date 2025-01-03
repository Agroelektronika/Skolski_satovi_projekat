

/*
  Esp32 mikrokontroler pokrece skolske satove i pali zvono u odredjenim trenucima. Komunikacija sa Android uredjajem preko WiFi za dobijanje sablona po kojem se zvoni i jos neke info. Postoje i tasteri za podesavanje satova.
  Za visoku preciznost merenja vremena koriste se 2 pristupa: povezivanje na lokalnu internet mrezu zarad dobijanja date/time informacije. Takodje vreme se meri unutrasnjim tajmerima. 
  Info sa interneta se moze koristiti za korekciju i sinhronizaciju.
  Mehanizam kazaljke radi tako sto se promene stanja 2 pina (toggle). Jedanput pin1 promena na HIGH i pin2 promena na LOW, pa onda obrnuto kad bude sledeca minuta. U medjuvremenu oba pina su ugasena zbog ustede.
  Postoji i 1 taster za zujalicu i 1 taster za resetovanje svih rasporeda. Rasporedi zvona se dobijaju od Androida. Ako je ukljuceno, automatski se ukljucuje zvono u odredjenin trenucima.

  Softver: Nemanja M.
  Hardver: Dragan M.
  Decembar 2024.

*/

#include <WiFi.h>
#include <string>
#include <time.h>
#include <map>
#include <vector>
#include <iterator>

using namespace std;


#define PIN_KAZALJKA1 33
#define PIN_KAZALJKA2 12
#define PIN_TASTER1 4
#define PIN_TASTER2 2
#define PIN_ZVONO 13

#define MAX_DUZINA 25
#define BR_POKUSAJA_POVEZIVANJA 20
#define DEBOUNCING_INTERVAL1 10000    // preciznije saltanje kazaljke, sporije, u blizini odredjene minute
#define DEBOUNCING_INTERVAL2 1000   // brze saltanje kazaljke, za brze dostizanje odredjenog sata i minute

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
const char* password_AP = "12345678";

char ssid_STA[MAX_DUZINA] = "123456";
char password_STA[MAX_DUZINA] = "tisovac2";

char NTP_server[MAX_DUZINA] = "pool.ntp.org";
const long gmtPomak = 3600U;       // Pomak u sekundama od GMT (za CET 3600)
const int letnje_aktivno = 3600U;   // Letnje računanje vremena (ako je aktivno)

WiFiClient client;
WiFiServer server(80);
String str_za_wifi = "";

uint8_t br_sekundi = 0U; // brojac prekida tajmera
Vreme v;
bool sinhronizovan = false;
bool prekid_tajmera = false;
bool prekid_tastera1 = false;
bool prekid_tastera2 = false;
bool prestupna_godina = false;
uint8_t status_sistema = STATUS_ERR_UNKNOWN_TIME;   // varijabla koja govori u kom stanju se nalazi sistem
bool povezan = false;
bool pin1_na_low = true; // info da pamti koji pin treba da se ugasi a koji da se upali kad dodje vreme da se posalje signal kazaljki sata. U medjuvremenu su ugasena oba pina zbog ustede. Signalizacija traje x ms.
uint8_t br_prekida_tajmera = 0U;
uint16_t br_debouncing_taster1 = 0U;
uint16_t br_debouncing_taster2 = 0U;

uint8_t sinhronizacija_sati = 6;    // vreme sinhronizacije sa sistemskim vremenom, preko interneta
uint8_t sinhronizacija_min = 30;
bool ukljuceno_automatsko_zvono = true;

std::map<uint8_t, uint8_t> raspored_zvona; // spisak svih trenutaka kad treba zvoniti

vector<uint8_t> raspored_zvona_sati;    // spisak svih sati kad treba zvoniti
vector<uint8_t> raspored_zvona_min;   // spisak svih minuta kad treba zvoniti (uslov da zvoni jeste da se poklapa i sat i minuta sa trenutnim vremenom)

int brojac_prekida = 0;

hw_timer_t* timer0 = NULL; 
portMUX_TYPE timerMux0 = portMUX_INITIALIZER_UNLOCKED;

portMUX_TYPE muxTaster1 = portMUX_INITIALIZER_UNLOCKED;                  //za external interrupt
portMUX_TYPE muxTaster2 = portMUX_INITIALIZER_UNLOCKED; 

void IRAM_ATTR onTimer0();    // prekidne rutine unutrasnjeg tajmera i spoljasnjih tastera
void IRAM_ATTR handleExternalTaster1();
void IRAM_ATTR handleExternalTaster2();
void vremenski_pomak();     // na osnovnu vremenskih info (sek, min, sat...) pomera ostale vrednosti u skladu sa pravilima
void sinhronizacija();   // povezivanje na internet server i sinhronizacija sa vremenskom zonom, korekcija akumulirane greske, jednom dnevno
void posalji_impuls_kazaljka();  // postavlja stanja pinova tako da pokrene mehanizam kazaljke sata
void upali_zvono();       // postavlja stanje pina tako da upali zvono
void posalji_stanje_WiFi();      // salje preko WiFi stanje Android uredjaju
void automatsko_podesavanje_sata(uint8_t sati, uint8_t min);    // automatsko podesavanje sata, prima vreme koje je na satu (sat je stao) i automatski ga podesava na trenutno
void obrada_stringa(String str);  // parsiranje primljenog stringa preko WiFi
void provera_string_sati(String str, uint8_t* sati, uint8_t* min);
void provera_string_ruter(String str, char ssid[], char password[]);

void setup() {

  setCpuFrequencyMhz(80);   // najniza brzina radnog takta
  Serial.begin(115200);

  timer0 = timerBegin(0, 80U, true);      // inicijalizacija tajmera koji meri vreme
  timerAttachInterrupt(timer0, &onTimer0, true);
  timerAlarmWrite(timer0, 1000000, true);     // korak od 1sek
  timerAlarmEnable(timer0);

  pinMode(PIN_TASTER1, INPUT_PULLUP);    // tasteri na ulaznim pinovima
  pinMode(PIN_TASTER2, INPUT_PULLUP);    //

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
  WiFi.begin(ssid_STA, password_STA); // inicijalizacija WiFi kao stanice, za povezivanje na druge WiFi mreze

  uint8_t br_pokusaja = 0U;
  while(WiFi.status() != WL_CONNECTED && br_pokusaja < BR_POKUSAJA_POVEZIVANJA){   // povezivanje na WiFi mrezu (koja ima internet), konacan broj puta, ako je neuspesno, izadji
    Serial.write(".");
    br_pokusaja += 1U;
    delay(500);
  }
  if(br_pokusaja < BR_POKUSAJA_POVEZIVANJA){
    povezan = true;
  }
  Serial.println(WiFi.localIP()); // IP od rutera

  configTime(gmtPomak, letnje_aktivno, NTP_server); // podesavanje vremena

  struct tm timeinfo;
  if(povezan)br_pokusaja = 0U;
  while (!getLocalTime(&timeinfo) && br_pokusaja < BR_POKUSAJA_POVEZIVANJA && povezan) {    // slanje zahteva ka internet serveru koji daje date/time
    br_pokusaja += 1U;
  }
  if(br_pokusaja < BR_POKUSAJA_POVEZIVANJA){
    status_sistema = STATUS_OK;
  }
  br_pokusaja = 0U;

  WiFi.disconnect(true);      // prekidanje veze sa ruterom kad se dobije potreban podatak
  delay(500);

  if (WiFi.softAP(ssid_AP, password_AP)) {  // inicijalizacija sopstvene WiFi mreze
      Serial.println("kreirana mreza");
  }  
  server.begin();     // pokretanje mreze

  delay(500U);

      // prepisivanje dobijenog sadrzaja sa internet servera koji daje datum/vreme
  v.sek = timeinfo.tm_sec;
  v.min = timeinfo.tm_min;
  v.sat = timeinfo.tm_hour;
  v.dan = timeinfo.tm_mday;
  v.dan_u_nedelji = timeinfo.tm_wday;
  v.mesec = timeinfo.tm_mon + 1U;
  v.god = timeinfo.tm_year + 1900U;

  Serial.println(String(v.dan) + "." + String(v.mesec) + "." + String(v.god));
  Serial.println(String(v.sat) + ":" + String(v.min) + ":" + String(v.sek));

}

void loop() {


  if(!client)client = server.available();   // provera da li je neko pokusao da se poveze na mrezu od ESP32, ako jeste vraca objekat client
  else{
    if(client.connected()){   // provera da li je veza aktivna i validna
      while (client.available()) {     //ako postoje podaci za citanje sa WiFi, available() vraca broj bajtova koji su primljeni od klijenta
        String str = client.readStringUntil('.');
        Serial.println(str);
        obrada_stringa(str);
        delay(2);
        String ack = "t";
        client.println(ack);

        delay(2);

      }
    }
    else{
      client.stop();      // ako je doslo do prekida veze,da se sacuva ovaj objekat, da ne bude u zaglavljenom stanju
      client = WiFiClient();    // reset klijenta
      Serial.println("Klijent prekinuo vezu");
    }
  }

  vremenski_pomak();
  if(v.sat == 6U && v.min == 30U && !sinhronizovan){
    sinhronizacija();
  }
  else if(v.sat == 6U && v.min == 31U && sinhronizovan){
    sinhronizovan = false;
  }
  
  if(prekid_tajmera){
   // Serial.println(String(v.dan) + "." + String(v.mesec) + "." + String(v.god));
   // Serial.println(String(v.sat) + ":" + String(v.min) + ":" + String(v.sek));
    if(br_prekida_tajmera > 5){
      digitalWrite(PIN_KAZALJKA1, LOW);   // nakon kraceg intervala od pocetka minute, oba pina staviti na LOW zbog ustede
      digitalWrite(PIN_KAZALJKA2, LOW);
      br_prekida_tajmera = 0U;
    }
    prekid_tajmera = false;
  }
  //Serial.println(br_debouncing_taster1);
  if(digitalRead(PIN_TASTER1) == LOW && br_debouncing_taster1 > DEBOUNCING_INTERVAL1){      // polling, ispitivanje da li su tasteri pritisnuti
    Serial.println("pritisak tastera 1");                                               // metoda prekida je onemogucena zbog suma u napajanju koje dolazi sa 220V, i izaziva prekide kad ne treba
    brojac_prekida += 1;
    posalji_impuls_kazaljka();
    br_debouncing_taster1 = 0;
  }

 /* if(digitalRead(PIN_TASTER2) == LOW && br_debouncing_taster2 > DEBOUNCING_INTERVAL2){
    Serial.println("pritisak tastera 2");
    brojac_prekida += 1;
    posalji_impuls();
    br_debouncing_taster2 = 0;
  }*/

  br_debouncing_taster1++;
  if(br_debouncing_taster1 > 28000) br_debouncing_taster1 = 27800;

  br_debouncing_taster2++;
  if(br_debouncing_taster2 > 28000) br_debouncing_taster2 = 27800;

}

void vremenski_pomak(){
  if(v.sek >= 60U){
    v.sek = 0U;
    v.min += 1U;
    posalji_impuls_kazaljka();
   // Serial.println("Broj prekida: " + String(brojac_prekida));
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

void posalji_impuls_kazaljka(){
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

void upali_zvono(){

}
void posalji_stanje_WiFi(){

}

void automatsko_podesavanje_sata(uint8_t sati, uint8_t min){

}

void provera_string_sati(String str, uint8_t* sati, uint8_t* min){
    String sati_str = "";
    String min_str = "";
    int indeks_delimitera = 0;
    indeks_delimitera = str.charAt(':');
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
    indeks_delimitera = str.charAt(':');
    int j = 0;
    for(int i = 1; i < indeks_delimitera; i++, j++){
      ssid[j] = str[i];
    }
    j = 0;
    for(int i = indeks_delimitera + 1; i < str.length(); i++, j++){
      password[j] = str[i];
    }
}

void obrada_stringa(String str){
  if(str[0] == 'a'){
    upali_zvono();
  }
  else if(str[0] == 'b'){
    posalji_stanje_WiFi();
  }
  else if(str[0] == 'c'){
    posalji_impuls_kazaljka();
  }
  else if(str[0] == 'd'){
    uint8_t sati = 0;
    uint8_t min = 0;
    provera_string_sati(str, &sati, &min);
    automatsko_podesavanje_sata(sati, min);
  }
  else if(str[0] == 'e'){
    uint8_t sati = 0;
    uint8_t min = 0;
    provera_string_sati(str, &sati, &min);
    sinhronizacija_sati = sati;
    sinhronizacija_min = min;
  }
  else if(str[0] == 'f'){
    provera_string_ruter(str, ssid_STA, password_STA);
  }
  else if(str[0] == 'g'){
    if(str[1] == '0'){
      ukljuceno_automatsko_zvono = false;
    }
    else if(str[1] == '1'){
      ukljuceno_automatsko_zvono = true;
    }
  }
  else if(str[0] == 'h'){
      char delimiter_zvona = '_';
      char delimiter_sat_min = ':';

      raspored_zvona_sati.clear();
      raspored_zvona_min.clear();

      raspored_zvona.clear();

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
        raspored_zvona[sat] = min;
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
/*
void IRAM_ATTR handleExternalTaster1(){
  portENTER_CRITICAL_ISR(&muxTaster1);
  prekid_tastera1 = true;
  br_debouncing_taster1 = 0;
  
  portEXIT_CRITICAL_ISR(&muxTaster1);
}

void IRAM_ATTR handleExternalTaster2(){
  portENTER_CRITICAL_ISR(&muxTaster2);
  prekid_tastera2 = true;
  br_debouncing_taster2 = 0;
  portEXIT_CRITICAL_ISR(&muxTaster2);
}
*/