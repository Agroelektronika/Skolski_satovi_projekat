package nemanja.milosevic.zvono;

import static androidx.core.content.ContextCompat.startActivity;

import static nemanja.milosevic.zvono.GlobalnaKlasa.aktivan_raspored;
import static nemanja.milosevic.zvono.GlobalnaKlasa.db;
import static nemanja.milosevic.zvono.GlobalnaKlasa.upisi_u_memoriju;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.Typeface;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageButton;
import android.widget.TextView;

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
                //GlobalnaKlasa.aktivan_raspored = element.getIme();
                //upisi_u_memoriju("aktivan_raspored", aktivan_raspored, kontekst);
                //ime_txt.setTypeface(null, Typeface.BOLD);
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
