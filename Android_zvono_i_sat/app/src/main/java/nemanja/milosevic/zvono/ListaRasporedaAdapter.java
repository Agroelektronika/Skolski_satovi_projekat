package nemanja.milosevic.zvono;

import static androidx.core.content.ContextCompat.startActivity;

import static nemanja.milosevic.zvono.GlobalnaKlasa.aktivan_raspored;
import static nemanja.milosevic.zvono.GlobalnaKlasa.db;
import static nemanja.milosevic.zvono.GlobalnaKlasa.dbHelper;
import static nemanja.milosevic.zvono.GlobalnaKlasa.upisi_u_memoriju;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.graphics.Color;
import android.graphics.Typeface;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.Toast;

import java.io.IOException;
import java.util.ArrayList;

public class ListaRasporedaAdapter extends BaseAdapter {

    private Context kontekst;
    private ArrayList<ElementListeRasporeda> elementi;

    public ListaRasporedaAdapter(Context c){
        this.kontekst = c;
        elementi = new ArrayList<ElementListeRasporeda>();
    }


    @Override
    public int getCount() {
        return elementi.size();
    }

    @Override
    public Object getItem(int position){
        Object rv = null;
        try {
            rv = elementi.get(position);
        } catch (IndexOutOfBoundsException e) {
            e.printStackTrace();
        }
        return rv;
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View view = convertView;
        Mreza mreza = Mreza.Instanca(kontekst);
        if(view == null){ //ako nije popunjen red, znaci da treba naduvati layout od nekog elementa
            LayoutInflater inflater = (LayoutInflater) kontekst.getSystemService(Context.LAYOUT_INFLATER_SERVICE);   //trazenje servisa od OS za naduvavanje layout-a od elementa u listu
            view = inflater.inflate(R.layout.lista_rasporeda_element, null);  // naduvavanje tj. smestanje layout-a od elementa u listu
        }
            //ako je popunjena lista, a doslo je do njenog pomeranja na ekranu, treba prikazati novi element
        ElementListeRasporeda element = (ElementListeRasporeda)getItem(position);
        TextView ime_txt = view.findViewById(R.id.txt_raspored);
        ImageButton imgBtnPosalji = view.findViewById(R.id.imgBtnPosalji);
        ImageButton imgBtnObrisi = view.findViewById(R.id.imgBtnObrisi);

        ime_txt.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent i = new Intent(kontekst, OdabirRasporedaZvona.class);
                i.putExtra("ime", element.getIme());
                kontekst.startActivity(i);
            }
        });

        imgBtnPosalji.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                try {


                    dbHelper = new BazaPodataka.Database(kontekst); // inicijalizacija baze
                    db = dbHelper.getReadableDatabase();
                    String[] kolone = {"kategorija", "ime"}; //spisak kolona koje su u SQL upitu ( koje treba procitati ) - COLUMN
                    String slanje = "h";
                    Cursor cursor = db.query("Zvona",   //tabela
                            kolone,
                            null,
                            null,
                            null,
                            null,
                            null);


                    while (cursor.moveToNext()) {    //iteriranje kroz tabelu dobijenu upitom
                        String kategorijaa = cursor.getString(cursor.getColumnIndexOrThrow("kategorija"));
                        String ime = cursor.getString(cursor.getColumnIndexOrThrow("ime"));
                        if (kategorijaa.equals(element.getIme())) {
                            slanje += ime;
                            slanje += '_';
                        }
                    }
                    slanje += ".";

                    cursor.close();
                    if (mreza.povezan && mreza.socketWiFi.isConnected()) {
                        if (GlobalnaKlasa.ukljucenoZvono) {
                            String finalSlanje = slanje;
                            new Thread(() -> {  // sve mrezne operacije u pozadinskoj niti
                                try {
                                    if (mreza.socketWiFi == null || mreza.socketWiFi.isClosed() || !mreza.socketWiFi.isConnected()) {
                                        mreza.poveziWiFi(); // Ponovno povezivanje
                                    }
                                    mreza.outputStream.write(finalSlanje.getBytes());
                                    mreza.outputStream.flush();
                                    aktivan_raspored = element.getIme();
                                    upisi_u_memoriju("aktivan_raspored", aktivan_raspored, kontekst);
                                } catch (IOException e) {
                                    throw new RuntimeException(e);
                                }
                            }).start();
                        } else {
                            Toast.makeText(kontekst, "Неуспешно. Искључено звоно.", Toast.LENGTH_SHORT).show();
                        }
                    }
                }catch (Exception e){
                    e.printStackTrace();
                    Toast.makeText(kontekst, "Прво додати звона", Toast.LENGTH_SHORT).show();

                }
            }

        });
        imgBtnObrisi.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String ime_za_brisanje = elementi.get(position).getIme();
                elementi.remove(position);
                db.execSQL("DELETE FROM Rasporedi WHERE ime = '" + ime_za_brisanje + "'");
                notifyDataSetChanged();
            }
        });

        ime_txt.setText(element.getIme());
        if(element.getAktivan()){
          //  ime_txt.setTypeface(null, Typeface.BOLD);
        }
        return view;

    }

    public void dodajElement(ElementListeRasporeda el){
        elementi.add(el);
        notifyDataSetChanged();
    }
    public void obrisiElement(ElementListeRasporeda el){
        elementi.remove(el);
    }



}
