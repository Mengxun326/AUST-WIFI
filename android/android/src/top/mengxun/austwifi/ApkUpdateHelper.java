package top.mengxun.austwifi;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Build;
import android.provider.Settings;

import androidx.core.content.FileProvider;

import org.json.JSONObject;

import java.io.File;
import java.util.List;

public final class ApkUpdateHelper {
    private static final String APK_MIME_TYPE = "application/vnd.android.package-archive";

    private ApkUpdateHelper() {
    }

    @SuppressWarnings("deprecation")
    public static String versionInfo(Context context) {
        JSONObject json = new JSONObject();
        try {
            if (context == null) {
                return json.toString();
            }

            PackageManager packageManager = context.getPackageManager();
            PackageInfo packageInfo = packageManager.getPackageInfo(context.getPackageName(), 0);
            long versionCode;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                versionCode = packageInfo.getLongVersionCode();
            } else {
                versionCode = packageInfo.versionCode;
            }

            json.put("packageName", context.getPackageName());
            json.put("versionName", packageInfo.versionName == null ? "" : packageInfo.versionName);
            json.put("versionCode", versionCode);
        } catch (Exception e) {
            try {
                json.put("error", e.getClass().getSimpleName());
            } catch (Exception ignored) {
            }
        }
        return json.toString();
    }

    public static boolean canInstallPackages(Context context) {
        if (context == null) {
            return false;
        }
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
            return true;
        }
        try {
            return context.getPackageManager().canRequestPackageInstalls();
        } catch (RuntimeException e) {
            return false;
        }
    }

    @SuppressWarnings("deprecation")
    public static String apkInfo(Context context, String apkPath) {
        JSONObject json = new JSONObject();
        try {
            if (context == null || apkPath == null || apkPath.trim().isEmpty()) {
                return json.toString();
            }

            PackageInfo packageInfo = context.getPackageManager()
                    .getPackageArchiveInfo(apkPath, 0);
            if (packageInfo == null) {
                json.put("error", "InvalidApk");
                return json.toString();
            }

            long versionCode;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                versionCode = packageInfo.getLongVersionCode();
            } else {
                versionCode = packageInfo.versionCode;
            }

            json.put("packageName", packageInfo.packageName == null ? "" : packageInfo.packageName);
            json.put("versionName", packageInfo.versionName == null ? "" : packageInfo.versionName);
            json.put("versionCode", versionCode);
        } catch (Exception e) {
            try {
                json.put("error", e.getClass().getSimpleName());
            } catch (Exception ignored) {
            }
        }
        return json.toString();
    }

    public static boolean openInstallPermissionSettings(Context context) {
        if (context == null) {
            return false;
        }

        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
            return true;
        }

        Intent intent = new Intent(
                Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES,
                Uri.parse("package:" + context.getPackageName())
        );
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        try {
            context.startActivity(intent);
            return true;
        } catch (RuntimeException e) {
            return openFallbackSecuritySettings(context);
        }
    }

    public static boolean installApk(Context context, String apkPath) {
        if (context == null || apkPath == null || apkPath.trim().isEmpty()) {
            return false;
        }
        if (!canInstallPackages(context)) {
            return false;
        }

        File apkFile = new File(apkPath);
        if (!apkFile.isFile()) {
            return false;
        }

        try {
            Uri apkUri = FileProvider.getUriForFile(
                    context,
                    context.getPackageName() + ".fileprovider",
                    apkFile
            );

            Intent intent = new Intent(Intent.ACTION_VIEW);
            intent.setDataAndType(apkUri, APK_MIME_TYPE);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            intent.putExtra(Intent.EXTRA_NOT_UNKNOWN_SOURCE, true);
            grantReadPermission(context, intent, apkUri);
            context.startActivity(intent);
            return true;
        } catch (RuntimeException e) {
            return false;
        }
    }

    private static void grantReadPermission(Context context, Intent intent, Uri apkUri) {
        List<ResolveInfo> handlers = context.getPackageManager()
                .queryIntentActivities(intent, PackageManager.MATCH_DEFAULT_ONLY);
        for (ResolveInfo handler : handlers) {
            if (handler.activityInfo != null && handler.activityInfo.packageName != null) {
                context.grantUriPermission(
                        handler.activityInfo.packageName,
                        apkUri,
                        Intent.FLAG_GRANT_READ_URI_PERMISSION
                );
            }
        }
    }

    private static boolean openFallbackSecuritySettings(Context context) {
        Intent fallback = new Intent(Settings.ACTION_SECURITY_SETTINGS);
        fallback.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        try {
            context.startActivity(fallback);
            return true;
        } catch (RuntimeException ignored) {
            return false;
        }
    }
}
