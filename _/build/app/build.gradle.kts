plugins {
    id("com.android.application")
}

// Release identity is source-controlled so F-Droid can rebuild a tagged commit
// without GitHub Actions environment variables. Keep the code monotonically
// increasing across both test APK and F-Droid releases.
val releaseVersionCode = 101
val releaseVersionName = "0.2.0"
val fdroidBuild = providers.gradleProperty("fdroidBuild").orNull == "true"

val wegertColorMarker = "/*__WEGERT_COLOR_CORE__*/"
val generatedWegertAssets = layout.buildDirectory.dir("generated/wegert-assets")
val assembleWegertShader = tasks.register("assembleWegertShader") {
    val template = rootProject.file("wegert.frag.in")
    val colorCore = rootProject.file("wegert_color.glsl")
    val output = generatedWegertAssets.map { it.file("wegert.frag") }

    inputs.files(template, colorCore)
    outputs.file(output)

    doLast {
        val templateText = template.readText()
        check(templateText.contains(wegertColorMarker)) {
            "Wegert fragment template is missing the coloring-core marker"
        }
        check(templateText.indexOf(wegertColorMarker) == templateText.lastIndexOf(wegertColorMarker)) {
            "Wegert fragment template must contain exactly one coloring-core marker"
        }

        val outputFile = output.get().asFile
        outputFile.parentFile.mkdirs()
        outputFile.writeText(templateText.replace(wegertColorMarker, colorCore.readText()))
    }
}

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
        manifestPlaceholders["appLabel"] = if (fdroidBuild) "zero & infinity" else "Wegert"

        // arm64-v8a is the main real phone/tablet target. armeabi-v7a keeps
        // the same native app installable on 32-bit Android/Android Go userspace,
        // and x86_64 is included so the same APK can be installed on CI emulators.
        ndk {
            abiFilters += listOf("arm64-v8a", "armeabi-v7a", "x86_64")
        }

        externalNativeBuild {
            cmake {
                arguments += listOf(
                    "-DANDROID_STL=none",
                    "-DWEGERT_USE_ICK_PREBUILT=${if (fdroidBuild) "OFF" else "ON"}",
                )
            }
        }
    }

    buildTypes {
        getByName("debug") {
            signingConfig = signingConfigs.getByName("stableDebug")
        }
    }

    sourceSets {
        getByName("main") {
            // AGP forbids Provider objects in SourceSet. Resolve the directory
            // eagerly here; the explicit preBuild dependency below carries the
            // generation ordering.
            assets.srcDir(generatedWegertAssets.get().asFile)
        }
    }

    externalNativeBuild {
        cmake {
            path = rootProject.file("CMakeLists.txt")
            version = "3.22.1"
        }
    }
}

tasks.named("preBuild").configure {
    dependsOn(assembleWegertShader)
}
