package top.mengxun.austwifi;

import android.Manifest;
import android.app.Activity;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ServiceInfo;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;

public final class AustWifiForegroundService extends Service {
    private static final String CHANNEL_ID = "aust_wifi_guard";
    private static final String CHANNEL_NAME = "AUST WiFi 后台守护";
    private static final int NOTIFICATION_ID = 32603;
    private static final int NOTIFICATION_PERMISSION_REQUEST_CODE = 32604;
    private static final long GUARD_TICK_INTERVAL_MS = 15_000L;

    private final Handler guardHandler = new Handler(Looper.getMainLooper());
    private final Runnable guardTickRunnable = new Runnable() {
        @Override
        public void run() {
            if (!guardLoopRunning) {
                return;
            }
            dispatchGuardTick();
            guardHandler.postDelayed(this, GUARD_TICK_INTERVAL_MS);
        }
    };
    private boolean guardLoopRunning = false;

    public static boolean start(Context context) {
        if (context == null) {
            return false;
        }

        try {
            Intent intent = new Intent(context, AustWifiForegroundService.class);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                context.startForegroundService(intent);
            } else {
                context.startService(intent);
            }
            return true;
        } catch (RuntimeException e) {
            return false;
        }
    }

    public static boolean stop(Context context) {
        if (context == null) {
            return false;
        }

        try {
            Intent intent = new Intent(context, AustWifiForegroundService.class);
            return context.stopService(intent);
        } catch (RuntimeException e) {
            return false;
        }
    }

    public static boolean requestNotificationPermission(Context context) {
        if (!(context instanceof Activity)) {
            return false;
        }
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU) {
            return false;
        }
        if (context.checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS)
                == PackageManager.PERMISSION_GRANTED) {
            return false;
        }

        try {
            ((Activity) context).requestPermissions(
                    new String[] { Manifest.permission.POST_NOTIFICATIONS },
                    NOTIFICATION_PERMISSION_REQUEST_CODE
            );
            return true;
        } catch (RuntimeException e) {
            return false;
        }
    }

    public static boolean hasNotificationPermission(Context context) {
        if (context == null) {
            return true;
        }
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU) {
            return true;
        }
        return context.checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS)
                == PackageManager.PERMISSION_GRANTED;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        ensureNotificationChannel();
        Notification notification = buildNotification();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            startForeground(
                    NOTIFICATION_ID,
                    notification,
                    ServiceInfo.FOREGROUND_SERVICE_TYPE_DATA_SYNC
            );
        } else {
            startForeground(NOTIFICATION_ID, notification);
        }
        startGuardLoop();
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        stopGuardLoop();
        stopForeground(STOP_FOREGROUND_REMOVE);
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private void ensureNotificationChannel() {
        NotificationManager manager = getSystemService(NotificationManager.class);
        if (manager == null || manager.getNotificationChannel(CHANNEL_ID) != null) {
            return;
        }

        NotificationChannel channel = new NotificationChannel(
                CHANNEL_ID,
                CHANNEL_NAME,
                NotificationManager.IMPORTANCE_LOW
        );
        channel.setDescription("保持 AUST WiFi 后台守护服务运行");
        manager.createNotificationChannel(channel);
    }

    private Notification buildNotification() {
        Intent launchIntent = getPackageManager().getLaunchIntentForPackage(getPackageName());
        PendingIntent pendingIntent = null;
        if (launchIntent != null) {
            pendingIntent = PendingIntent.getActivity(
                    this,
                    0,
                    launchIntent,
                    PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE
            );
        }

        Notification.Builder builder = new Notification.Builder(this, CHANNEL_ID);

        builder.setSmallIcon(R.mipmap.ic_launcher)
                .setContentTitle("AUST WiFi 正在后台守护")
                .setContentText("正在定时检查 WiFi，并在需要时触发自动登录")
                .setOngoing(true)
                .setShowWhen(false);

        if (pendingIntent != null) {
            builder.setContentIntent(pendingIntent);
        }

        return builder.build();
    }

    private void startGuardLoop() {
        if (guardLoopRunning) {
            return;
        }
        guardLoopRunning = true;
        guardHandler.post(guardTickRunnable);
    }

    private void stopGuardLoop() {
        guardLoopRunning = false;
        guardHandler.removeCallbacks(guardTickRunnable);
    }

    private static void dispatchGuardTick() {
        try {
            nativeGuardTick();
        } catch (UnsatisfiedLinkError | RuntimeException e) {
            // The service may be restarted by Android before Qt registers the native side.
        }
    }

    private static native void nativeGuardTick();
}
