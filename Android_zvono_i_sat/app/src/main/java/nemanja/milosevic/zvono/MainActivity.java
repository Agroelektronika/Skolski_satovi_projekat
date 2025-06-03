package nemanja.milosevic.zvono;

import static nemanja.milosevic.zvono.GlobalnaKlasa.korisnicko_ime;
import static nemanja.milosevic.zvono.GlobalnaKlasa.lozinka;
import static nemanja.milosevic.zvono.GlobalnaKlasa.preference;
import static nemanja.milosevic.zvono.GlobalnaKlasa.ucitaj_iz_memorije;
import static nemanja.milosevic.zvono.GlobalnaKlasa.upisi_u_memoriju;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

public class MainActivity extends AppCompatActivity {
// u Main Activity se vrsi login, samo prvi put, jednom kad se uloguje, automatski se vrsi svaki sledeci put kad se upali aplikacija


    String sifra = "jswomvjknmc";
    boolean ulogovan = false;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
       // EdgeToEdge.enable(this);
        setContentView(R.layout.activity_main);

        Button btnLogin = findViewById(R.id.btnConfirmLogin);
        EditText editUsername = findViewById(R.id.username_edit);
        EditText editPassword = findViewById(R.id.password_edit);
        String logovan = ucitaj_iz_memorije("logovan", MainActivity.this);
        ulogovan = Boolean.parseBoolean(logovan);
        // na startu se proverava da li korisnik vec ulogovan
        if(ulogovan){
            Intent i = new Intent(MainActivity.this, HomeActivity.class);
            startActivity(i);
        }

        btnLogin.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String username = String.valueOf(editUsername.getText());
                String password = String.valueOf(editPassword.getText());

                if(username.equals(korisnicko_ime) && password.equals(lozinka)){
                    upisi_u_memoriju("sifra", lozinka, MainActivity.this);
                    ulogovan = true;
                    upisi_u_memoriju("logovan", String.valueOf(ulogovan), MainActivity.this);
                    Intent i = new Intent(MainActivity.this, HomeActivity.class);
                    startActivity(i);
                }
                else{
                    ulogovan = false;
                    System.out.println(username);
                    System.out.println(password);
                    System.out.println(sifra);
                    upisi_u_memoriju("logovan", String.valueOf(ulogovan), MainActivity.this);
                    Toast.makeText(MainActivity.this, "Погрешна шифра", Toast.LENGTH_SHORT).show();
                }
            }
        });

    }




/*
    public String ucitaj_iz_memorije(String kljuc){
        preference =  getSharedPreferences("preference", Context.MODE_PRIVATE);
        return preference.getString(kljuc, "");
    }
    public void upisi_u_memoriju(String kljuc, String vrednost){
        preference =  getSharedPreferences("preference", Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = preference.edit();
        editor.putString(kljuc, vrednost);
        editor.apply();
    }

 */
}