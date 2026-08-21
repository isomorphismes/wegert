plugins {
    id("com.android.application")
}

val requestedPlayVersionCode = System.getenv("PLAY_VERSION_CODE")
val buildNumber = when {
    requestedPlayVersionCode == null -> System.getenv("GITHUB_RUN_NUMBER")?.toIntOrNull() ?: 1
    else -> requestedPlayVersionCode.toIntOrNull()
        ?.takeIf { it in 1..2_100_000_000 }
        ?: error("PLAY_VERSION_CODE must be between 1 and 2100000000")
}
val releaseVersionName = System.getenv("PLAY_VERSION_NAME") ?: "0.1.$buildNumber"

val uploadKeystorePath = System.getenv("ANDROID_UPLOAD_KEYSTORE_PATH")
val uploadKeystorePassword = System.getenv("ANDROID_UPLOAD_KEYSTORE_PASSWORD")
val uploadKeyAlias = System.getenv("ANDROID_UPLOAD_KEY_ALIAS")
val uploadKeyPassword = System.getenv("ANDROID_UPLOAD_KEY_PASSWORD")
val uploadSigningConfigured = listOf(
    uploadKeystorePath,
    uploadKeystorePassword,
    uploadKeyAlias,
    uploadKeyPassword,
).all { !it.isNullOrBlank() }

android {
    namespace = "org.isomorphisms.wegert"
    compileSdk = 36
    ndkVersion = "29.0.14206865"

    signingConfigs {
        create("stableDebug") {
            storeFile = file("wegert-debug.keystore")
            storePassword = "wegert-debug"
            keyAlias = "wegert-debug"
            keyPassword = "wegert-debug"
            storeType = "pkcs12"
        }

        if (uploadSigningConfigured) {
            create("playUpload") {
                storeFile = file(uploadKeystorePath!!)
                storePassword = uploadKeystorePassword
                keyAlias = uploadKeyAlias
                keyPassword = uploadKeyPassword
            }
        }
    }

    defaultConfig {
        applicationId = "org.isomorphisms.wegert"
        minSdk = 26
        targetSdk = 36
        versionCode = buildNumber
        versionName = releaseVersionName

        // arm64-v8a is the main real phone/tablet target. armeabi-v7a keeps
        // the same native app installable on 32-bit Android/Android Go userspace,
        // and x86_64 is included so the same APK can be installed on CI emulators.
        ndk {
            abiFilters += listOf("arm64-v8a", "armeabi-v7a", "x86_64")
            debugSymbolLevel = "FULL"
        }

        externalNativeBuild {
            cmake {
                arguments += listOf("-DANDROID_STL=none")
            }
        }
    }

    buildTypes {
        getByName("debug") {
            signingConfig = signingConfigs.getByName("stableDebug")
        }
        getByName("release") {
            signingConfigs.findByName("playUpload")?.let {
                signingConfig = it
            }
        }
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}
