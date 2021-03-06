// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.weblayer_private;

import android.content.Context;
import android.text.TextUtils;

import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.weblayer.WebLayer;

/**
 * Helper for metrics_browsertest.cc
 */
@JNINamespace("weblayer")
class MetricsTestHelper {
    private static class TestGmsBridge extends GmsBridge {
        private final boolean mUserConsent;

        public TestGmsBridge(boolean userConsent) {
            mUserConsent = userConsent;
        }

        @Override
        public boolean canUseGms() {
            return true;
        }

        @Override
        public void setSafeBrowsingHandler() {
            // We don't have this specialized service here.
        }

        @Override
        public void queryMetricsSetting(Callback<Boolean> callback) {
            ThreadUtils.assertOnUiThread();
            callback.onResult(mUserConsent);
        }

        @Override
        public void logMetrics(byte[] data) {
            MetricsTestHelperJni.get().onLogMetrics(data);
        }
    }

    @CalledByNative
    private static void installTestGmsBridge(boolean userConsent) {
        GmsBridge.injectInstance(new TestGmsBridge(userConsent));
    }

    @CalledByNative
    private static void createProfile(String name) {
        Context appContext = ContextUtils.getApplicationContext();
        WebLayer weblayer = WebLayer.loadSync(appContext);

        String nameOrNull = null;
        if (!TextUtils.isEmpty(name)) nameOrNull = name;
        weblayer.getProfile(nameOrNull);
    }

    @CalledByNative
    private static void destroyProfile(String name) {
        Context appContext = ContextUtils.getApplicationContext();
        WebLayer weblayer = WebLayer.loadSync(appContext);

        String nameOrNull = null;
        if (!TextUtils.isEmpty(name)) nameOrNull = name;
        weblayer.getProfile(nameOrNull).destroy();
    }

    @CalledByNative
    private static void removeTestGmsBridge() {
        GmsBridge.injectInstance(null);
    }

    @NativeMethods
    interface Natives {
        void onLogMetrics(byte[] data);
    }
}
