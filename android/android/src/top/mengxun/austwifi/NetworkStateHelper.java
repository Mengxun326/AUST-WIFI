package top.mengxun.austwifi;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.wifi.WifiInfo;
import android.os.Build;

import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

public final class NetworkStateHelper {
    private static final int WIFI_PERMISSION_REQUEST_CODE = 32602;
    private static final String UNKNOWN_SSID = "<unknown ssid>";

    private NetworkStateHelper() {
    }

    public static String networkState(Context context) {
        JSONObject json = new JSONObject();
        try {
            boolean hasFineLocation = hasPermission(context, Manifest.permission.ACCESS_FINE_LOCATION);
            boolean hasNearbyWifi = Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU
                    || hasPermission(context, Manifest.permission.NEARBY_WIFI_DEVICES);

            Network activeNetwork = activeNetwork(context);
            boolean wifiConnected = isWifiConnected(context);
            String ssid = "";
            if (wifiConnected) {
                ssid = cleanSsid(readSsid(context));
            }

            json.put("sdk", Build.VERSION.SDK_INT);
            json.put("wifiConnected", wifiConnected);
            json.put("ssid", ssid);
            json.put("networkKey", activeNetwork == null ? "" : activeNetwork.toString());
            json.put("hasFineLocation", hasFineLocation);
            json.put("hasNearbyWifi", hasNearbyWifi);
            json.put("canReadSsid", !ssid.isEmpty());
            json.put("missingPermissions", missingPermissionsText(context));
        } catch (Exception e) {
            try {
                json.put("error", e.getClass().getSimpleName());
            } catch (Exception ignored) {
            }
        }
        return json.toString();
    }

    public static boolean requestNetworkPermissions(Context context) {
        if (!(context instanceof Activity)) {
            return false;
        }

        List<String> missing = new ArrayList<>();
        if (!hasPermission(context, Manifest.permission.ACCESS_FINE_LOCATION)) {
            missing.add(Manifest.permission.ACCESS_FINE_LOCATION);
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU
                && !hasPermission(context, Manifest.permission.NEARBY_WIFI_DEVICES)) {
            missing.add(Manifest.permission.NEARBY_WIFI_DEVICES);
        }
        if (missing.isEmpty()) {
            return false;
        }

        ((Activity) context).requestPermissions(
                missing.toArray(new String[0]),
                WIFI_PERMISSION_REQUEST_CODE
        );
        return true;
    }

    private static Network activeNetwork(Context context) {
        ConnectivityManager connectivityManager =
                (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        if (connectivityManager == null) {
            return null;
        }
        return connectivityManager.getActiveNetwork();
    }

    private static boolean isWifiConnected(Context context) {
        ConnectivityManager connectivityManager =
                (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        if (connectivityManager == null) {
            return false;
        }

        Network activeNetwork = connectivityManager.getActiveNetwork();
        NetworkCapabilities capabilities = connectivityManager.getNetworkCapabilities(activeNetwork);
        return capabilities != null && capabilities.hasTransport(NetworkCapabilities.TRANSPORT_WIFI);
    }

    private static String readSsid(Context context) {
        try {
            ConnectivityManager connectivityManager =
                    (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
            if (connectivityManager == null) {
                return "";
            }

            NetworkCapabilities capabilities =
                    connectivityManager.getNetworkCapabilities(connectivityManager.getActiveNetwork());
            if (capabilities != null && capabilities.getTransportInfo() instanceof WifiInfo) {
                return ((WifiInfo) capabilities.getTransportInfo()).getSSID();
            }

            return "";
        } catch (SecurityException e) {
            return "";
        }
    }

    private static String cleanSsid(String ssid) {
        if (ssid == null) {
            return "";
        }

        String trimmed = ssid.trim();
        if (trimmed.isEmpty() || UNKNOWN_SSID.equalsIgnoreCase(trimmed)) {
            return "";
        }
        if (trimmed.length() >= 2 && trimmed.startsWith("\"") && trimmed.endsWith("\"")) {
            return trimmed.substring(1, trimmed.length() - 1);
        }
        return trimmed;
    }

    private static String missingPermissionsText(Context context) {
        List<String> missing = new ArrayList<>();
        if (!hasPermission(context, Manifest.permission.ACCESS_FINE_LOCATION)) {
            missing.add("定位");
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU
                && !hasPermission(context, Manifest.permission.NEARBY_WIFI_DEVICES)) {
            missing.add("附近 WiFi 设备");
        }
        return String.join("、", missing);
    }

    private static boolean hasPermission(Context context, String permission) {
        if (context == null) {
            return false;
        }
        return context.checkSelfPermission(permission) == PackageManager.PERMISSION_GRANTED;
    }
}
