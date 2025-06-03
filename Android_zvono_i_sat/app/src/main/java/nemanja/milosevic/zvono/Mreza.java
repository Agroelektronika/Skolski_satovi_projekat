package nemanja.milosevic.zvono;

import android.Manifest;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;

import android.graphics.Color;
import android.widget.Button;
import android.widget.Toast;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;



import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.Reader;
import java.net.InetAddress;
import java.net.Socket;
import java.util.Set;
import java.util.UUID;

/*
 *   Klasa koja obuhvata sva potrebna polja i metode za obavljanje mreznih operacija (povezivanje i komunikacija preko WiFi i Bluetooth)
 *
 * */

public class Mreza {


    public Reader reader = null;    // WiFi, BT
    InputStream inputStream = null; // WiFi, BT
    BluetoothAdapter btadapter = null;  //BT
    Socket socketWiFi = null;//WiFi
    Set<BluetoothDevice> pairedDevices = null; //BT
    BluetoothDevice targetDevice = null; //BT
    public OutputStream outputStream = null;    //WiFi, BT

    static Context kontekst;   // dobijanje info koja se aktivnost izvrsava u trenutku pozivanja ovih metoda

    public boolean povezan = false;



    private static Mreza mreza;

    public static Mreza Instanca(Context c){   //deljeni objekat medju aktivnostima,//prosledjivanje konteksta ( aktivnost ) u konstruktoru klase
        if(mreza == null){          // singleton pattern
            mreza = new Mreza();
        }
        kontekst = c;
        return mreza;
    }

    public void traziDozvolu(Context c){
        //trazenje dozvole od korisnika da dozvoli povezivanje BT
        if (ContextCompat.checkSelfPermission(c, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            // Dozvola nije odobrena, zatražite je
            ActivityCompat.requestPermissions((Activity) c, new String[]{Manifest.permission.BLUETOOTH_CONNECT}, 1);
        }
        if (ContextCompat.checkSelfPermission(c, Manifest.permission.ACCESS_WIFI_STATE) != PackageManager.PERMISSION_GRANTED) {
            // Dozvola nije odobrena, zatražite je
            ActivityCompat.requestPermissions((Activity) c, new String[]{Manifest.permission.ACCESS_WIFI_STATE}, 1);
        }
        // Dozvola je već odobrena, možete nastaviti
    }





    public void poveziWiFi() throws IOException {

        String serverIp = "192.168.4.1";
        int serverPort = 80;
        InetAddress serverAddr = InetAddress.getByName(serverIp);
        socketWiFi = new Socket(serverAddr, serverPort);
        // BufferedReader breader = new BufferedReader(new InputStreamReader(socket.getInputStream()));
        reader = new InputStreamReader(socketWiFi.getInputStream());
        try {
            outputStream = socketWiFi.getOutputStream();
        }catch (Exception e){
            e.printStackTrace();
        }
        if(socketWiFi.isConnected()){
            povezan = true;
        }

    }

    public void resetujMrezu(){ //kad se izlazi iz aktivnosti
        reader = null;
        inputStream = null;
        btadapter = null;
        socketWiFi = null;//wifi
        pairedDevices = null;
        targetDevice = null;
        outputStream = null;
    }



}