package com.android.settings; 
  
import android.app.Activity;  
import android.app.AlertDialog;  
import android.app.Dialog;  
import android.app.AlertDialog.Builder;  
import android.content.DialogInterface;  
import android.os.Bundle;  
import android.util.Log;  
import android.view.View;  
import android.widget.Button;  
import android.widget.EditText;  
import android.widget.Toast;
import android.os.SystemProperties;
  
public class ChangeBoaeBoardVersionActivity extends Activity {  
      
    private final int SING_CHOICE_DIALOG = 1;  
    @Override  
    public void onCreate(Bundle savedInstanceState) {  
        super.onCreate(savedInstanceState);  
        setContentView(R.layout.change_board_version_layout);  
        Log.e("ChangeBoaeBoardVersionActivity", "----------------------------------------------------------");  
        Button button = (Button) findViewById(R.id.buttonlemaker);  
        View.OnClickListener listener = new View.OnClickListener() {  
              
            @Override  
            public void onClick(View view) {  
                showDialog(SING_CHOICE_DIALOG);  
            }  
        };  
        button.setOnClickListener(listener);  
    }  
      
    @Override  
    protected Dialog onCreateDialog(int id) {  
        Dialog dialog = null;  
        switch(id) {  
            case SING_CHOICE_DIALOG:  
                Builder builder = new AlertDialog.Builder(this);  
                builder.setIcon(R.drawable.appwidget_button_left);  
                builder.setTitle("Change Your Base Board Version");  
                final ChoiceOnClickListener choiceListener =   
                    new ChoiceOnClickListener();  
                builder.setSingleChoiceItems(R.array.hobby, 0, choiceListener);  
                  
                DialogInterface.OnClickListener btnListener =   
                    new DialogInterface.OnClickListener() {  
                        @Override  
                        public void onClick(DialogInterface dialogInterface, int which) {  
                               
                            int choiceWhich = choiceListener.getWhich(); 

                            String hobbyStr =  getResources().getStringArray(R.array.hobby)[choiceWhich]; 
                            Toast.makeText(getApplicationContext(), hobbyStr, Toast.LENGTH_SHORT).show();  
                            if(choiceWhich == 0)
                            		{SystemProperties.set("ro.product.board","lemaker_guitar_bba");SystemProperties.set("ctl.start", "bbaSelect");}
                            else if(choiceWhich == 1)
                            		{SystemProperties.set("ro.product.board","lemaker_guitar_bbb");SystemProperties.set("ctl.start", "bbbSelect");}
                            else if(choiceWhich == 2)
                            		{SystemProperties.set("ro.product.board","lemaker_guitar_bbb_plus");SystemProperties.set("ctl.start", "bbbplusSelect");}	
                            else if(choiceWhich == 3)
                            		{SystemProperties.set("ro.product.board","lemaker_guitar_bbc");SystemProperties.set("ctl.start", "bbcSelect");}	 
                            else if(choiceWhich == 4)
                            		{SystemProperties.set("ro.product.board","lemaker_guitar_bbd");SystemProperties.set("ctl.start", "bbdSelect");}	                            		                           		  
                        }  
                    };  
                builder.setPositiveButton("Confirm", btnListener);  
                dialog = builder.create();  
                break;  
        }  
        return dialog;  
    }  
      
    private class ChoiceOnClickListener implements DialogInterface.OnClickListener {  
  
        private int which = 0;  
        @Override  
        public void onClick(DialogInterface dialogInterface, int which) {  
            this.which = which;  
        }  
          
        public int getWhich() {  
            return which;  
        }  
    }  
}  