package com.aircast.util

import android.content.ComponentCallbacks2
import android.content.ComponentCallbacks2.TRIM_MEMORY_COMPLETE
import android.content.Context
import android.content.res.Configuration
import android.util.Log
import java.util.concurrent.atomic.AtomicBoolean

/**
 * Proxies [ComponentCallbacks2] and [NetworkObserver.Listener] calls to a
 * weakly referenced [imageLoader].
 *
 * This prevents the system from having a strong reference to the [imageLoader], which allows
 * it be freed naturally by the garbage collector. If the [imageLoader] is freed, it unregisters
 * its callbacks.
 */
public class SystemCallbacks(
        private val context: Context,
        isNetworkObserverEnabled: Boolean
) : ComponentCallbacks2, NetworkObserver.Listener {
    private val networkObserver = if (isNetworkObserverEnabled) {
        NetworkObserver(context, this)
    } else {
        EmptyNetworkObserver()
    }

    @Volatile
    private var _isOnline = networkObserver.isOnline
    private val _isShutdown = AtomicBoolean(false)

    val isOnline get() = _isOnline
    val isShutdown get() = _isShutdown.get()

    fun register() {
        context.registerComponentCallbacks(this)
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        Log.d(TAG, "onConfigurationChanged()  newConfig = $newConfig")
    }

    override fun onTrimMemory(level: Int) {
        Log.d(TAG, "onTrimMemory()   level = $level")
    }

    override fun onLowMemory() = onTrimMemory(TRIM_MEMORY_COMPLETE)

    override fun onConnectivityChange(isOnline: Boolean) {
        Log.d(TAG, "onConnectivityChange()  isOnline = $isOnline")
        _isOnline = isOnline
    }

    fun shutdown() {
        if (_isShutdown.getAndSet(true)) return
        context.unregisterComponentCallbacks(this)
        networkObserver.shutdown()
    }

    companion object {
        private const val TAG = "NetworkObserver"
        private const val ONLINE = "ONLINE"
        private const val OFFLINE = "OFFLINE"
    }
}