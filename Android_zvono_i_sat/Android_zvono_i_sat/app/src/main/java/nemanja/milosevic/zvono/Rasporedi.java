package nemanja.milosevic.zvono;

import static nemanja.milosevic.zvono.GlobalnaKlasa.db;
import static nemanja.milosevic.zvono.GlobalnaKlasa.dbHelper;
import static nemanja.milosevic.zvono.GlobalnaKlasa.ucitaj_iz_memorije;

import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.os.Bundle;
import android.text.InputType;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import android.database.Cursor;




public class Rasporedi extends AppCompatActivity {

    String unos = "";
    ListaRasporedaAdapter adapter;
    private void unosParametara() {
        AlertDialog.Builder builder = new AlertDialog.Builder(Rasporedi.this);
        builder.setTitle("Унос");

        LinearLayout layout = new LinearLayout(Rasporedi.this);
        layout.setOrientation(LinearLayout.VERTICAL);

        final EditText input1 = new EditText(Rasporedi.this);
        input1.setHint("Назив распореда");
        input1.setInputType(InputType.TYPE_CLASS_TEXT);
        layout.addView(input1);

        builder.setView(layout);
        db = dbHelper.getWritableDatabase();

        builder.setPositiveButton("Потврди", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                try {
                    unos = input1.getText().toString();
                    adapter.dodajElement(new ElementListeRasporeda(unos, false));
                    db.execSQL("INSERT INTO Rasporedi Values('" + unos + "')");
                    adapter.notifyDataSetChanged();
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        });
        builder.setNegativeButton("Откажи", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.cancel();
            }
        });
        builder.show();

    }


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_rasporedi);

        adapter = new ListaRasporedaAdapter(this);
        ListView lista = findViewById(R.id.lista);
        lista.setAdapter(adapter);
        TextView prazna = findViewById(R.id.prazna1);
        lista.setEmptyView(prazna);

        ImageButton btnDodaj = findViewById(R.id.imgBtnDodajRaspored);
        btnDodaj.setBackgroundColor(Color.WHITE);
        GlobalnaKlasa.aktivan_raspored = ucitaj_iz_memorije("aktivan_raspored", Rasporedi.this);
        dbHelper = new BazaPodataka.Database(Rasporedi.this); // inicijalizacija baze
        db = dbHelper.getReadableDatabase();
        //dbHelper.obrisi_tabelu_rasporeda_zvona(db);
        //dbHelper.napravi_tabelu_rasporedi_zvona(db);
        String[] kolone = {"ime"}; //spisak kolona koje su u SQL upitu ( koje treba procitati ) - COLUMN

        Cursor cursor = db.query("Rasporedi",   //tabela
                kolone,
                null,
                null,
                null,
                null,
                null);


        while (cursor.moveToNext()){    //iteriranje kroz tabelu dobijenu upitom
            String ime = cursor.getString(cursor.getColumnIndexOrThrow("ime"));
            boolean da_li_je_aktivan = false;
            if(ime.equals(GlobalnaKlasa.aktivan_raspored)){
                da_li_je_aktivan = true;
            }
            adapter.dodajElement(new ElementListeRasporeda(ime, da_li_je_aktivan));
        }

        cursor.close();

        btnDodaj.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                unosParametara();
            }
        });

        lista.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                /*Intent i = new Intent(Rasporedi.this, OdabirRasporedaZvona.class);
                i.putExtra("ime", "prvi");
                startActivity(i);*/
            }
        });

    }
}