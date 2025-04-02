# Add project specific ProGuard rules here.
# By default, the flags in this file are appended to flags specified
# in C:\Android\sdk/tools/proguard/proguard-android.txt
# You can edit the include path and order by changing the proguardFiles
# directive in build.gradle.
#
# For more details, see
#   http://developer.android.com/guide/developing/tools/proguard.html

# Add any project specific keep options here:

# If your project uses WebView with JS, uncomment the following
# and specify the fully qualified class name to the JavaScript interface
# class:
#-keepclassmembers class fqcn.of.javascript.interface.for.webview {
#   public *;
#}


-optimizationpasses 5
-dontusemixedcaseclassnames
-dontskipnonpubliclibraryclasses
-dontpreverify
-verbose
-optimizations !code/simplification/arithmetic,!field/*,!class/merging/*

# Keep all Fragments in this package, which are used by reflection.

-keep class com.rockchip.mediacenter.** { *; }
-keep class io.github.deweyreed.** { *; }
-keep class com.aircast.jni.** { *; }
-keep class com.yueshi.mediarender.jni.** { *; }
-keep class com.aircast.app.App { *; }
-keep class com.aircast.source.AirplayMirrorSource { *; }
-keep class com.aircast.dlna.DMRBridge.DMRClass { *; }
-keep class com.aircast.settings.Setting { *; }

-keep class com.aircast.app.ui.LollipopFixedWebView { *; }
-keep public enum com.aircast.settings.Setting$** {
    **[] $VALUES;
    public *;
}
-dontwarn com.aircast.dlna.plugins.**

# Keep click responders


# Keep all Fragments in this package, which are used by reflection.
-keep class com.android.settings.** { *; }
 

-keep public class * extends android.app.Activity
-keep public class * extends android.app.Application
-keep public class * extends android.app.Service
-keep public class * extends android.content.BroadcastReceiver
-keep public class * extends android.content.ContentProvider
-keep public class * extends android.app.backup.BackupAgentHelper
-keep public class * extends android.preference.Preference


-keepclassmembers class ** {
   public static *** parse(***);
}

-keepclasseswithmembernames class * {
    native <methods>;
}

-keepclasseswithmembers class * {
    public <init>(android.content.Context, android.util.AttributeSet);
}

-keepclasseswithmembers class * {
    public <init>(android.content.Context, android.util.AttributeSet, int);
}

-keepclassmembers class * extends android.app.Activity {
   public void *(android.view.View);
}

-keepclassmembers enum * {
    public static **[] values();
    public static ** valueOf(java.lang.String);
}

-keep class * implements android.os.Parcelable {
  public static final android.os.Parcelable$Creator *;
}

##---------------Begin: proguard configuration for Gson  ----------
# Gson uses generic type information stored in a class file when working with fields. Proguard
# removes such information by default, so configure it to keep all of it.
-keepattributes Signature
-keepattributes InnerClasses

# For using GSON @Expose annotation
-keepattributes *Annotation*

# Gson specific classes
-dontwarn sun.misc.**
#-keep class com.google.gson.stream.** { *; }

# Application classes that will be serialized/deserialized over Gson
-keep class com.google.gson.examples.android.model.** { *; }

# Prevent proguard from stripping interface information from TypeAdapterFactory,
# JsonSerializer, JsonDeserializer instances (so they can be used in @JsonAdapter)
-keep class * implements com.google.gson.TypeAdapterFactory
-keep class * implements com.google.gson.JsonSerializer
-keep class * implements com.google.gson.JsonDeserializer

##---------------End: proguard configuration for Gson  ----------
#########################################################################################
# Retain generated class which implement Unbinder.
-keep public class * implements butterknife.Unbinder { public <init>(**, android.view.View); }

# Prevent obfuscation of types which use ButterKnife annotations since the simple name
# is used to reflectively look up the generated ViewBinding.
-keep class butterknife.*
-keepclasseswithmembernames class * { @butterknife.* <methods>; }
-keepclasseswithmembernames class * { @butterknife.* <fields>; }

#########################################################################################
## Square Otto specific rules ##
## https://square.github.io/otto/ ##

-keepattributes *Annotation*
-keepclassmembers class ** {
    @com.squareup.otto.Subscribe public *;
    @com.squareup.otto.Produce public *;
}

#########################################################################################
#ijkplayer
-keep class tv.danmaku.ijk.media.player.** {*;}
-keep class tv.danmaku.ijk.media.player.IjkMediaPlayer{*;}
-keep class tv.danmaku.ijk.media.player.ffmpeg.FFmpegApi{*;}
#########################################################################################

-keep class org.apache.log4j.** { *; }

-dontwarn org.rockchip.mediacenter.**
-dontwarn com.rockchip.mediacenter.**
-dontwarn com.android.internal.**
-dontwarn org.apache.log4j.**
-dontwarn sun.misc.Unsafe.**

#########################################################################################

-dontwarn com.tencent.bugly.**
-keep class com.tencent.bugly.**{*;}
-keep class com.tencent.tinker.**{*;}
-keep class android.support.**{*;}
#########################################################################################
-keep class com.google.android.material.** { *; }

-dontwarn com.google.android.material.**
-dontnote com.google.android.material.**

-dontwarn androidx.**
-keep class androidx.** { *; }
-keep interface androidx.** { *; }
-keep class androidx.appcompat.widget.** { *; }


-keep class com.github.druk.dnssd.**{*;}

-keepclassmembers class * {
   public <init> (org.json.JSONObject);
}
-keepclassmembers enum * {
    public static **[] values();
    public static ** valueOf(java.lang.String);
}

-keep class com.tencent.** {*;}
-keep class com.cicada.player.** {*;}

-dontwarn com.squareup.otto.**
-dontwarn com.rockchip.mediacenter.**
-dontwarn com.alibaba.fastjson.**
-dontwarn com.nirvana.tools.**
-dontwarn com.tencent.**
-dontwarn okhttp3.**
-dontwarn okio.**


-keep class org.json.**{*;}
