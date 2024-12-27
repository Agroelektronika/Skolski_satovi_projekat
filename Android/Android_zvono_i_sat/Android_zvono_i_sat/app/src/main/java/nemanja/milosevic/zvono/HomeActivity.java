package nemanja.milosevic.zvono;

import android.graphics.Color;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.ImageButton;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
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
        imgBtnWifi.setBackgroundColor(Color.RED);
        imgBtnBell.setBackgroundColor(Color.RED);
        imgBtnWifi.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                imgBtnWifi.setBackgroundColor(Color.GREEN);
            }
        });
    }
}