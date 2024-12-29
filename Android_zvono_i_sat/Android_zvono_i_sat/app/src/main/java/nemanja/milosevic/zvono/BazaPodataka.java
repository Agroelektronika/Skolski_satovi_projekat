package nemanja.milosevic.zvono;

import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

import androidx.annotation.Nullable;

public class BazaPodataka {
    public static class Database extends SQLiteOpenHelper {

        private static final String ime_baze = "RasporediZvona.db";
        private static final int verzija = 1;

        public Database(@Nullable Context context) {
            super(context, ime_baze, null, verzija);
        }

        public Database(@Nullable Context context, @Nullable String name, @Nullable SQLiteDatabase.CursorFactory factory, int version) {
            super(context, name, factory, version);
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
          //  napravi_tabelu_rasporedi_zvona(db);
            // izvrsavanje SQL komande za kreiranje baze podataka
        }

        public void napravi_tabelu_rasporedi_zvona(SQLiteDatabase db){
           // db.execSQL("DROP TABLE Korisnici");
            db.execSQL("CREATE TABLE Rasporedi (ime TEXT)");
        }
        public void obrisi_tabelu_rasporeda_zvona(SQLiteDatabase db){
            db.execSQL("DROP TABLE Rasporedi");
        }
        public void napravi_tabelu_zvona(SQLiteDatabase db){
            db.execSQL("CREATE TABLE Zvona (kategorija TEXT, ime TEXT)");
        }
        public void obrisi_tabelu_zvona(SQLiteDatabase db){
            db.execSQL("DROP TABLE Zvona");
        }


        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        }
    }
}
