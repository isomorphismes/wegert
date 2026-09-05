// SPDX-License-Identifier: GPL-3.0-or-later

#include <android/log.h>
#include <jni.h>

#define WEGERT_JNI_TAG "Wegert"
#define WEGERT_JNI_SENTINEL 47047

JNIEXPORT jint JNICALL
Java_org_isomorphisms_wegert_WegertActivity_jniProbe(JNIEnv *environment,
                                                      jclass activity_class) {
    (void)environment;
    (void)activity_class;

    __android_log_print(ANDROID_LOG_INFO, WEGERT_JNI_TAG,
                        "jniProbe=%d", WEGERT_JNI_SENTINEL);
    return WEGERT_JNI_SENTINEL;
}
