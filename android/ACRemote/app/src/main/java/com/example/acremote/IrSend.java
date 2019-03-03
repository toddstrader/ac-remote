package com.example.acremote;

import android.content.res.AssetManager;
import android.os.AsyncTask;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;

public class IrSend extends AsyncTask<String, Void, Integer> {

    private static JSONObject json = null;

    public static void readJson (AssetManager amgr) {
        try {
            InputStream is = amgr.open("ac-codes.json");
            int size = is.available();
            byte[] buffer = new byte[size];
            is.read(buffer);
            is.close();
            String codes = new String(buffer, "UTF-8");
            json = new JSONObject(codes);
        } catch (IOException e) {
            e.printStackTrace();
        } catch (JSONException e) {
            e.printStackTrace();
        }

        return;
    }

    protected Integer doInBackground(String... host) {
        Socket socket = null;
        try {
            socket = new Socket("192.168.43.27", 4178);
            OutputStream outStream = socket.getOutputStream();
            JSONObject turnOff = json.getJSONObject("turn-off");
            JSONArray data = turnOff.getJSONArray("data");
            String dataStr = data.join(",");
            outStream.write(dataStr.getBytes());
            outStream.flush();
            outStream.close();
            socket.close();
        } catch (IOException e) {
            e.printStackTrace();
        } catch (JSONException e) {
            e.printStackTrace();
        }

        return 0;
    }
}
