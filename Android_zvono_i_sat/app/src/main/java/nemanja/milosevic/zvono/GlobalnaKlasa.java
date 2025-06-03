package nemanja.milosevic.zvono;

import android.content.Context;
import android.content.SharedPreferences;
import android.database.sqlite.SQLiteDatabase;

public class GlobalnaKlasa {

    public static String korisnicko_ime = "admin";
    public static String lozinka = "jswomvjknmc";
    public static SharedPreferences preference;
    public static BazaPodataka.Database dbHelper; //inicijalizator baze podataka
    public static SQLiteDatabase db;   //instance za otvaranje baze podataka i upis i citanje    int broj_rasporeda_zvona = 0;   // vazna info kad se pamte podaci, da znamo koliko ih ima
    public static String aktivan_raspored = "-";
    public static boolean ukljucenoZvono = true;
    public static String vreme_sinhronizacije = "06:30";
    public static String ssid_ruter = "";
    public static String password_ruter = "";

    public static String ucitaj_iz_memorije(String kljuc, Context kontekst){
        preference =  kontekst.getSharedPreferences("preference", Context.MODE_PRIVATE);
        return preference.getString(kljuc, "");
    }
    public static void upisi_u_memoriju(String kljuc, String vrednost, Context kontekst){
        preference =  kontekst.getSharedPreferences("preference", Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = preference.edit();
        editor.putString(kljuc, vrednost);
        editor.apply();
    }

    public static boolean provera_stringa_sat(String unos){
        int indeks_delimitera = unos.indexOf(':');
        boolean dobar_unos = true;
        if(indeks_delimitera < 1 || indeks_delimitera > 2) {
            dobar_unos = false;
        }
        if(indeks_delimitera == 1 || indeks_delimitera == 2){
            boolean dobar_unos1 = true;
            boolean dobar_unos2 = true;
            for(int i = 0; i < indeks_delimitera; i++){
                if(unos.charAt(i) < '0' || unos.charAt(i) > '9'){
                    dobar_unos1 = false;
                }
            }
            for(int i = indeks_delimitera + 1; i < unos.length(); i++){
                if(unos.charAt(i) < '0' || unos.charAt(i) > '9'){
                    dobar_unos2 = false;
                }
                if(i > indeks_delimitera + 2){
                    dobar_unos2 = false;
                }
            }

            if(!dobar_unos1 || !dobar_unos2){
                dobar_unos = false;
            }
        }
        return dobar_unos;
    }

}
