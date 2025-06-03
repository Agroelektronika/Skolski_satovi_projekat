package nemanja.milosevic.zvono;

import static nemanja.milosevic.zvono.GlobalnaKlasa.aktivan_raspored;
import static nemanja.milosevic.zvono.GlobalnaKlasa.password_ruter;
import static nemanja.milosevic.zvono.GlobalnaKlasa.provera_stringa_sat;
import static nemanja.milosevic.zvono.GlobalnaKlasa.ssid_ruter;
import static nemanja.milosevic.zvono.GlobalnaKlasa.ucitaj_iz_memorije;
import static nemanja.milosevic.zvono.GlobalnaKlasa.upisi_u_memoriju;
import static nemanja.milosevic.zvono.GlobalnaKlasa.vreme_sinhronizacije;

import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.Toolbar;
import android.view.Menu;
import android.view.MenuItem;
import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import java.io.IOException;
import java.util.Calendar;

public class HomeActivity extends AppCompatActivity {


    Mreza mreza = Mreza.Instanca(HomeActivity.this);    // deljeni objekat, medju aktivnostima
    ImageButton imgBtnWifi;
    ImageButton imgBtnBell;
    ImageButton imgBtnConfirm;
    ImageButton imgBtnConfirmRouter;
    ImageButton imgBtnInfo;
    ImageButton imgBtnRasporedi;
    ImageButton imgBtnStanje;
    ImageButton imgBtnSync;
    Button btnPlus1;
    Button btnAuto;
    Button btnResult;
    EditText zaostala_kazaljka;
    EditText sinhronizacija_vreme;
    EditText ssid_router;
    EditText password_router;
    Switch ukljuceno_zvono;
    TextView aktivan_raspored_txt;


    public class MreznaNit implements Runnable {    //sporedna nit, za mreznu komunikaciju
        public void run() {
            try {
                btnResult = findViewById(R.id.btnRezDijagnostike);
                if(!mreza.povezan) {
                    mreza.traziDozvolu(HomeActivity.this);
                    imgBtnWifi = findViewById(R.id.imgBtnWifi);
                    mreza.poveziWiFi();
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
            runOnUiThread(new Runnable() {          //AZURIRANJE UI ELEMENATA, U GLAVNOJ NITI UI
                @Override
                public void run() {
                    if(mreza.povezan) {
                        imgBtnWifi.setBackgroundColor(Color.GREEN);
                    }
                    else{
                        Toast.makeText(HomeActivity.this, "Неуспешно повезивање", Toast.LENGTH_SHORT).show();
                    }
                }
            });
            zaostala_kazaljka = findViewById(R.id.clock_smart);

            int b = 0;
            String str = "";
            try {

                while (mreza.povezan && (b = mreza.reader.read()) != -1) {       // ACK
                    char c = (char)b;
                    str += c;
                    System.out.println(c);
                    if(c == 't'){
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(HomeActivity.this, "Послато", Toast.LENGTH_SHORT).show();
                            }
                        });
                    }
                    else if(c == '0'){
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                btnResult.setBackgroundColor(Color.GREEN);
                            }
                        });
                    }
                    else if(c == '1'){
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                btnResult.setBackgroundColor(Color.RED);
                            }
                        });
                    }
                    else if(c == '2'){
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                btnResult.setBackgroundColor(Color.YELLOW);
                            }
                        });
                    }
                    else if(c == '.'){
                        String finalStr = str;
                        runOnUiThread(new Runnable() {          //AZURIRANJE UI ELEMENATA, U GLAVNOJ NITI UI
                            @Override
                            public void run() {
                                zaostala_kazaljka.setText(finalStr);
                            }
                        });
                        str = "";
                    }
                }



            }catch (Exception e){
                e.printStackTrace();
            }

        }
    }


            @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_home);

        try {

            imgBtnWifi = findViewById(R.id.imgBtnWifi);
            imgBtnRasporedi = findViewById(R.id.imgBtnRaspored2);
            imgBtnBell = findViewById(R.id.imgBtnBell);
            imgBtnConfirm = findViewById(R.id.imgBtnCorrect);
            imgBtnConfirmRouter = findViewById(R.id.imgBtnCorrect2);
            imgBtnInfo = findViewById(R.id.imgBtnInfo);
            imgBtnStanje = findViewById(R.id.imgBtnCheck);
            imgBtnSync = findViewById(R.id.imgBtnSync);
            btnPlus1 = findViewById(R.id.btnPoterajZaJedan);
            btnAuto = findViewById(R.id.btnSmartTeranjeKazaljke);
            zaostala_kazaljka = findViewById(R.id.clock_smart);
            sinhronizacija_vreme = findViewById(R.id.sync_clock);
            ssid_router = findViewById(R.id.ssid);
            password_router = findViewById(R.id.password);
            ukljuceno_zvono = findViewById(R.id.ukljuceno_zvono);
            aktivan_raspored_txt = findViewById(R.id.info_o_rasporedu_txt);
            btnResult = findViewById(R.id.btnRezDijagnostike);

            imgBtnInfo.setBackgroundColor(ContextCompat.getColor(HomeActivity.this, R.color.blue));
            imgBtnWifi.setBackgroundColor(Color.RED);
            imgBtnBell.setBackgroundColor(Color.RED);
            imgBtnConfirm.setBackgroundColor(Color.WHITE);
            imgBtnSync.setBackgroundColor(Color.WHITE);
            imgBtnConfirmRouter.setBackgroundColor(Color.WHITE);
            imgBtnRasporedi.setBackgroundColor(ContextCompat.getColor(this, R.color.blue));
            aktivan_raspored = ucitaj_iz_memorije("aktivan_raspored", this);
            if (aktivan_raspored.equals("")) aktivan_raspored = "-";
            aktivan_raspored_txt.setText(aktivan_raspored);
            GlobalnaKlasa.vreme_sinhronizacije = ucitaj_iz_memorije("sinhronizacija_vreme", this);
            GlobalnaKlasa.ssid_ruter = ucitaj_iz_memorije("ssid_ruter", this);
            GlobalnaKlasa.password_ruter = ucitaj_iz_memorije("password_ruter", this);

            sinhronizacija_vreme.setText(vreme_sinhronizacije);
            ssid_router.setText(ssid_ruter);
            password_router.setText(password_ruter);

            imgBtnWifi.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (!mreza.povezan) {
                        Thread t = new Thread(new MreznaNit());   //POZIVANJE THREAD-A I NJEGOVO STARTOVANJE
                        t.start();
                    }
                }
            });
            imgBtnRasporedi.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    Intent i = new Intent(HomeActivity.this, Rasporedi.class);
                    startActivity(i);
                }
            });


            imgBtnBell.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (mreza.povezan && mreza.socketWiFi.isConnected()) {
                        new Thread(() -> {  // sve mrezne operacije u pozadinskoj niti
                            String slanje = "a.";
                            try {
                                if (mreza.socketWiFi == null || mreza.socketWiFi.isClosed() || !mreza.socketWiFi.isConnected()) {
                                    mreza.poveziWiFi(); // Ponovno povezivanje
                                }
                                mreza.outputStream.write(slanje.getBytes());
                                mreza.outputStream.flush();
                            } catch (IOException e) {
                                throw new RuntimeException(e);
                            }
                        }).start();
                    } else {
                        Toast.makeText(HomeActivity.this, "Неуспешно", Toast.LENGTH_SHORT).show();
                        imgBtnWifi.setBackgroundColor(Color.RED);
                        mreza.povezan = false;
                    }
                }
            });

            imgBtnStanje.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (mreza.povezan && mreza.socketWiFi.isConnected()) {
                        new Thread(() -> {
                            String slanje = "b.";
                            try {
                                if (mreza.socketWiFi == null || mreza.socketWiFi.isClosed() || !mreza.socketWiFi.isConnected()) {
                                    mreza.poveziWiFi(); // Ponovno povezivanje
                                }
                                mreza.outputStream.write(slanje.getBytes());
                                mreza.outputStream.flush();
                            } catch (IOException e) {
                                throw new RuntimeException(e);
                            }
                        }).start();
                    } else {
                        Toast.makeText(HomeActivity.this, "Неуспешно", Toast.LENGTH_SHORT).show();
                        imgBtnWifi.setBackgroundColor(Color.RED);
                        mreza.povezan = false;
                    }
                }
            });

            btnResult.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {

                }
            });

            imgBtnSync.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (mreza.povezan && mreza.socketWiFi.isConnected()) {
                        new Thread(() -> {  // sve mrezne operacije u pozadinskoj niti
                            Calendar vreme = Calendar.getInstance();
                            int god = vreme.get(Calendar.YEAR);
                            int mesec = vreme.get(Calendar.MONTH);
                            int dan = vreme.get(Calendar.DAY_OF_MONTH);
                            int sati = vreme.get(Calendar.HOUR_OF_DAY);
                            int min = vreme.get(Calendar.MINUTE);
                            int sek = vreme.get(Calendar.SECOND);
                            String slanje = "j" + sati + ":" + min + ":" + sek + ":" + god + ":" + mesec + ":" + dan + ":_.";
                            try {
                                if (mreza.socketWiFi == null || mreza.socketWiFi.isClosed() || !mreza.socketWiFi.isConnected()) {
                                    mreza.poveziWiFi(); // Ponovno povezivanje
                                }
                                mreza.outputStream.write(slanje.getBytes());
                                mreza.outputStream.flush();
                            } catch (IOException e) {
                                throw new RuntimeException(e);
                            }
                        }).start();
                    } else {
                        Toast.makeText(HomeActivity.this, "Неуспешно", Toast.LENGTH_SHORT).show();
                        imgBtnWifi.setBackgroundColor(Color.RED);
                        mreza.povezan = false;
                    }
                }
            });

            btnPlus1.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (mreza.povezan && mreza.socketWiFi.isConnected()) {
                        new Thread(() -> {
                            String slanje = "i.";   // treba c
                            try {
                                if (mreza.socketWiFi == null || mreza.socketWiFi.isClosed() || !mreza.socketWiFi.isConnected()) {
                                    mreza.poveziWiFi(); // Ponovno povezivanje
                                }
                                mreza.outputStream.write(slanje.getBytes());
                                mreza.outputStream.flush();
                            } catch (IOException e) {
                                throw new RuntimeException(e);
                            }
                        }).start();
                    } else {
                        Toast.makeText(HomeActivity.this, "Неуспешно", Toast.LENGTH_SHORT).show();
                        imgBtnWifi.setBackgroundColor(Color.RED);
                        mreza.povezan = false;
                    }
                }
            });

            btnAuto.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (mreza.povezan && mreza.socketWiFi.isConnected()) {

                        new Thread(() -> {

                            String sati = zaostala_kazaljka.getText().toString();
                            if (provera_stringa_sat(sati)) {
                                String slanje = "d" + sati + ".";
                                try {
                                    if (mreza.socketWiFi == null || mreza.socketWiFi.isClosed() || !mreza.socketWiFi.isConnected()) {
                                        mreza.poveziWiFi(); // Ponovno povezivanje
                                    }
                                    mreza.outputStream.write(slanje.getBytes());
                                    mreza.outputStream.flush();
                                } catch (IOException e) {
                                    throw new RuntimeException(e);
                                }
                            } else {
                                runOnUiThread(new Runnable() {          //AZURIRANJE UI ELEMENATA, U GLAVNOJ NITI UI
                                    @Override
                                    public void run() {
                                        Toast.makeText(HomeActivity.this, "Неправилан унос. Унесите у облику HH:MM", Toast.LENGTH_LONG).show();
                                    }
                                });
                            }
                        }).start();

                    } else {
                        Toast.makeText(HomeActivity.this, "Неуспешно", Toast.LENGTH_SHORT).show();
                        imgBtnWifi.setBackgroundColor(Color.RED);
                        mreza.povezan = false;
                    }
                }
            });

            imgBtnConfirm.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (mreza.povezan && mreza.socketWiFi.isConnected()) {
                        new Thread(() -> {
                            String sati = sinhronizacija_vreme.getText().toString();
                            if (provera_stringa_sat(sati)) {
                                String slanje = "e" + sati + ".";
                                try {
                                    if (mreza.socketWiFi == null || mreza.socketWiFi.isClosed() || !mreza.socketWiFi.isConnected()) {
                                        mreza.poveziWiFi(); // Ponovno povezivanje
                                    }
                                    mreza.outputStream.write(slanje.getBytes());
                                    mreza.outputStream.flush();
                                    upisi_u_memoriju("sinhronizacija_vreme", sati, HomeActivity.this);
                                } catch (IOException e) {
                                    throw new RuntimeException(e);
                                }
                            } else {
                                runOnUiThread(new Runnable() {          //AZURIRANJE UI ELEMENATA, U GLAVNOJ NITI UI
                                    @Override
                                    public void run() {
                                        Toast.makeText(HomeActivity.this, "Неправилан унос. Унесите у облику HH:MM", Toast.LENGTH_LONG).show();
                                    }
                                });
                            }
                        }).start();
                    } else {
                        Toast.makeText(HomeActivity.this, "Неуспешно", Toast.LENGTH_SHORT).show();
                        imgBtnWifi.setBackgroundColor(Color.RED);
                        mreza.povezan = false;
                    }
                }
            });

            imgBtnConfirmRouter.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (mreza.povezan && mreza.socketWiFi.isConnected()) {
                        new Thread(() -> {
                            String ssid = ssid_router.getText().toString();
                            String password = password_router.getText().toString();
                            String slanje = "f" + ssid + "_" + password + ".";
                            try {
                                if (mreza.socketWiFi == null || mreza.socketWiFi.isClosed() || !mreza.socketWiFi.isConnected()) {
                                    mreza.poveziWiFi(); // Ponovno povezivanje
                                }
                                mreza.outputStream.write(slanje.getBytes());
                                mreza.outputStream.flush();
                                upisi_u_memoriju("password_ruter", password, HomeActivity.this);
                                upisi_u_memoriju("ssid_ruter", ssid, HomeActivity.this);
                            } catch (IOException e) {
                                throw new RuntimeException(e);
                            }
                        }).start();
                    } else {
                        Toast.makeText(HomeActivity.this, "Неуспешно", Toast.LENGTH_SHORT).show();
                        imgBtnWifi.setBackgroundColor(Color.RED);
                        mreza.povezan = false;
                    }
                }
            });

            ukljuceno_zvono.setOnCheckedChangeListener((buttonView, isChecked) -> {
                if (mreza.povezan && mreza.socketWiFi.isConnected()) {
                    new Thread(() -> {
                        String slanje = "g1.";
                        if (isChecked) {
                            slanje = "g1.";
                            aktivan_raspored_txt.setText(aktivan_raspored);
                            GlobalnaKlasa.ukljucenoZvono = true;
                            //Toast.makeText(this, "Switch is ON", Toast.LENGTH_SHORT).show();
                        } else {
                            slanje = "g0.";
                            aktivan_raspored_txt.setText("-");
                            GlobalnaKlasa.ukljucenoZvono = false;
                            //Toast.makeText(this, "Switch is OFF", Toast.LENGTH_SHORT).show();
                        }
                        try {
                            if (mreza.socketWiFi == null || mreza.socketWiFi.isClosed() || !mreza.socketWiFi.isConnected()) {
                                mreza.poveziWiFi(); // Ponovno povezivanje
                            }
                            mreza.outputStream.write(slanje.getBytes());
                            mreza.outputStream.flush();
                            upisi_u_memoriju("ukljucenoZvono", String.valueOf(GlobalnaKlasa.ukljucenoZvono), this);
                        } catch (IOException e) {
                            throw new RuntimeException(e);
                        }
                    }).start();
                } else {
                    //Toast.makeText(HomeActivity.this, "Неуспешно", Toast.LENGTH_SHORT).show();
                    imgBtnWifi.setBackgroundColor(Color.RED);
                    mreza.povezan = false;
                }
            });

            imgBtnInfo.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    Intent i = new Intent(HomeActivity.this, InfoActivity.class);
                    startActivity(i);
                }
            });
        }catch (Exception e){
            e.printStackTrace();
        }
    }


    @Override
    protected void onStart() {
        super.onStart();
        GlobalnaKlasa.ukljucenoZvono = Boolean.parseBoolean(ucitaj_iz_memorije("ukljucenoZvono", this));
        ukljuceno_zvono.setChecked(GlobalnaKlasa.ukljucenoZvono);
        aktivan_raspored = ucitaj_iz_memorije("aktivan_raspored", this);
        if(aktivan_raspored.equals("")) aktivan_raspored = "-";
        aktivan_raspored_txt.setText(aktivan_raspored);
        if(!GlobalnaKlasa.ukljucenoZvono) aktivan_raspored_txt.setText("-");;
    }
}