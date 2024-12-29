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
    public static String aktivan_raspored = "";
    int broj_zvona = 0;

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

}
