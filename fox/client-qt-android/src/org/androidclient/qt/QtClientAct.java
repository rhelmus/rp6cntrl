package org.androidclient.qt;

import java.util.List;
import android.util.Log;
import android.os.Bundle;
import com.nokia.qt.android.QtActivity;
import com.nokia.qt.common.QtLibraryLoader;

public class QtClientAct extends QtActivity
{
    static final String TAG="QtClient";

    public QtClientAct()
    {
        super(
            "client-qt-android", // Name of the library containing your Qt application
                                     // (without "lib" and ".so")
            true,  // Use "hard" exit? (Don't leave preloaded binaries in memory;
                   // enable this option if your application does not restart
                   // because static / global variables are not reinitialized.)
            true,  // Pause app when activity is not running
            false  // Don't use keepalive service
        );
        // setPlugin("QtAndroidNoGl"); // QtAndroidNoGl is the default plugin.
        addQtLibrary(QtLibraryLoader.QtLibrary.QtNetwork);
    }
}
