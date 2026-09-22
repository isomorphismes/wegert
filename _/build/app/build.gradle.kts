import java.util.Properties

plugins {
    id("com.android.application")
}

val releaseProperties = Properties().apply {
    rootProject.file("fdroid/release.properties").inputStream().use { load(it) }
}
fun releaseProperty(name: String): String =
    releaseProperties.getProperty(name) ?: error("missing fdroid/release.properties key: $name")

val releasePackageId = releaseProperty("packageId")
val releaseVersionCode = releaseProperty("versionCode").toInt()
val releaseVersionName = releaseProperty("versionName")
val releaseMinSdk = releaseProperty("minSdk").toInt()
val releaseCompileSdk = releaseProperty("compileSdk").toInt()
val releaseTargetSdk = releaseProperty("targetSdk").toInt()
val releaseBuildTools = releaseProperty("buildTools")
val releaseCmakeVersion = releaseProperty("cmake")
val releaseNdkVersion = releaseProperty("ndk")
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
    namespace = releasePackageId
    compileSdk = releaseCompileSdk
    buildToolsVersion = releaseBuildTools
    ndkVersion = releaseNdkVersion

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
        applicationId = releasePackageId
        minSdk = releaseMinSdk
        targetSdk = releaseTargetSdk
        versionCode = releaseVersionCode
        versionName = releaseVersionName
        manifestPlaceholders["appLabel"] = if (fdroidBuild) "zero & infinity" else "Wegert"

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
            assets.srcDir(generatedWegertAssets.get().asFile)
        }
    }

    externalNativeBuild {
        cmake {
            path = rootProject.file("CMakeLists.txt")
            version = releaseCmakeVersion
        }
    }
}

tasks.named("preBuild").configure {
    dependsOn(assembleWegertShader)
}
