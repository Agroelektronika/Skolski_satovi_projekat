package nemanja.milosevic.zvono;

import static nemanja.milosevic.zvono.GlobalnaKlasa.db;
import static nemanja.milosevic.zvono.GlobalnaKlasa.dbHelper;

import android.content.DialogInterface;
import android.content.Intent;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.graphics.Color;
import android.os.Bundle;
import android.text.InputType;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
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

public class OdabirRasporedaZvona extends AppCompatActivity {



    String unos = "";
    String kategorija = "";
    ArrayAdapter<String> adapter;

    private void unosParametara() {
        AlertDialog.Builder builder = new AlertDialog.Builder(OdabirRasporedaZvona.this);
        builder.setTitle("Унос звона");

        LinearLayout layout = new LinearLayout(OdabirRasporedaZvona.this);
        layout.setOrientation(LinearLayout.VERTICAL);

        final EditText input1 = new EditText(OdabirRasporedaZvona.this);
        input1.setHint("XX:YY");
        input1.setInputType(InputType.TYPE_CLASS_TEXT);
        layout.addView(input1);

        builder.setView(layout);

        builder.setPositiveButton("Потврди", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                try {
                    String unos = input1.getText().toString();
                    db.execSQL("INSERT INTO Zvona (kategorija, ime) VALUES('" + kategorija + "', '" + unos + "')");
                    adapter.add(unos);
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
        setContentView(R.layout.activity_odabir_rasporeda_zvona);
        Intent i = getIntent();
        kategorija = i.getStringExtra("ime");
        TextView txtIme = findViewById(R.id.ime_txt);
        txtIme.setText(kategorija);

        adapter = new ArrayAdapter<String>(this, android.R.layout.simple_list_item_1);
        ListView listaZvona = findViewById(R.id.listaZvona);
        listaZvona.setAdapter(adapter);
        TextView prazna = findViewById(R.id.prazna);
        listaZvona.setEmptyView(prazna);
        ImageButton btnDodaj = findViewById(R.id.imgBtnDodajZvono);
        btnDodaj.setBackgroundColor(Color.WHITE);

        dbHelper = new BazaPodataka.Database(OdabirRasporedaZvona.this); // inicijalizacija baze
        db = dbHelper.getReadableDatabase();
        //dbHelper.napravi_tabelu_zvona(db);

        String[] kolone = {"kategorija", "ime"}; //spisak kolona koje su u SQL upitu ( koje treba procitati ) - COLUMN

        Cursor cursor = db.query("Zvona",   //tabela
                kolone,
                null,
                null,
                null,
                null,
                null);


        while (cursor.moveToNext()){    //iteriranje kroz tabelu dobijenu upitom
            String kategorijaa = cursor.getString(cursor.getColumnIndexOrThrow("kategorija"));
            String ime = cursor.getString(cursor.getColumnIndexOrThrow("ime"));
            if(kategorijaa.equals(kategorija)) {
                adapter.add(ime);
            }
        }

        cursor.close();

        btnDodaj.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //  Intent i = new Intent(Rasporedi.this, OdabirRasporedaZvona.class);
                //  i.putExtra("ime", "prvi");
                //  startActivity(i);
                unosParametara();
            }
        });

        listaZvona.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                String zaBrisanje = adapter.getItem(position);
                db.execSQL("DELETE FROM Zvona WHERE ime = '" + zaBrisanje + "'");
                adapter.remove(zaBrisanje);
                adapter.notifyDataSetChanged();
            }
        });


    }
}