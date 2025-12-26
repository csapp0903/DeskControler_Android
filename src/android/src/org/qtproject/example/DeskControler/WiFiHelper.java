// android/src/org/qtproject/example/DeskControler/WiFiHelper.java
package org.qtproject.example.DeskControler;

import android.content.Context;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.util.Log;

public class WiFiHelper {
    public static boolean connectToWiFi(Context context, String ssid, String password) {
		try {
            WifiManager wifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
            if (wifiManager == null) return false;

            if (!wifiManager.isWifiEnabled() && !wifiManager.setWifiEnabled(true)) {
                return false;
            }
            
            WifiConfiguration config = new WifiConfiguration();
            config.SSID = "\"" + ssid + "\"";
            config.preSharedKey = "\"" + password + "\"";
            config.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA_PSK);
			config.status = WifiConfiguration.Status.ENABLED;
            
            int netId = wifiManager.addNetwork(config);
            if (netId == -1) return false;
            
			boolean enableSuccess = wifiManager.enableNetwork(netId, true);
			boolean saveSuccess = wifiManager.saveConfiguration();
			
            return true;
        } catch (Exception e) {
            return false;
        }
    }
}