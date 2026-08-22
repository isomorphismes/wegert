plugins {
    id("com.android.application")
}

// Release identity is source-controlled so F-Droid can rebuild a tagged commit
// without GitHub Actions environment variables. Version code 100 is above the
// run-number-based debug builds already published from this repository.
val releaseVersionCode = 100
val releaseVersionName = "0.1.100"

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
    }

    defaultConfig {
        applicationId = "org.isomorphisms.wegert"
        minSdk = 26
        targetSdk = 36
        versionCode = releaseVersionCode
        versionName = releaseVersionName

        // arm64-v8a is the main real phone/tablet target. armeabi-v7a keeps
        // the same native app installable on 32-bit Android/Android Go userspace,
        // and x86_64 is included so the same APK can be installed on CI emulators.
        ndk {
            abiFilters += listOf("arm64-v8a", "armeabi-v7a", "x86_64")
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
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}
