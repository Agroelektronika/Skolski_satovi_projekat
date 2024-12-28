package nemanja.milosevic.zvono;

import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.Toolbar;
import android.view.Menu;
import android.view.MenuItem;
import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

public class HomeActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_home);

        ImageButton imgBtnWifi = findViewById(R.id.imgBtnWifi);
        ImageButton imgBtnBell = findViewById(R.id.imgBtnBell);
        ImageButton imgBtnConfirm = findViewById(R.id.imgBtnCorrect);
        ImageButton imgBtnConfirmRouter = findViewById(R.id.imgBtnCorrect2);
        ImageButton imgBtnInfo = findViewById(R.id.imgBtnInfo);
        ImageButton imgBtnRasporedi = findViewById(R.id.imgBtnRaspored2);

        imgBtnRasporedi.setBackgroundColor(ContextCompat.getColor(this, R.color.blue));
        imgBtnInfo.setBackgroundColor(ContextCompat.getColor(this, R.color.blue));
        imgBtnWifi.setBackgroundColor(Color.RED);
        imgBtnBell.setBackgroundColor(Color.RED);
        imgBtnConfirm.setBackgroundColor(Color.WHITE);
        imgBtnConfirmRouter.setBackgroundColor(Color.WHITE);

        imgBtnWifi.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                imgBtnWifi.setBackgroundColor(Color.GREEN);
            }
        });
        imgBtnRasporedi.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent i = new Intent(HomeActivity.this, Rasporedi.class);
                startActivity(i);
            }
        });
    }
}