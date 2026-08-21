# Google Play release

Wegert's permanent Android package ID is `org.isomorphisms.wegert`. The release
build targets API 36 and uses NDK r29, whose native libraries support 16 KiB
memory pages. CI also retains native debug symbols for Play crash reports.

The existing `app/wegert-debug.keystore` remains a public, test-only identity.
It must never sign a Play upload.

## GitHub environment and secrets

Create a protected environment named `google-play` with:

- `ANDROID_UPLOAD_KEYSTORE_BASE64`
- `ANDROID_UPLOAD_KEYSTORE_PASSWORD`
- `ANDROID_UPLOAD_KEY_ALIAS`
- `ANDROID_UPLOAD_KEY_PASSWORD`
- `GOOGLE_PLAY_SERVICE_ACCOUNT_JSON`

The Android secrets describe a private upload key. Enroll the app in Play App
Signing so Google retains the separate app-signing key used for device APKs.

## First and later uploads

Google's publishing API cannot create the Play application or perform its first
manual upload. Create the app with the exact package ID above, complete its store
and policy declarations, run `Google Play app bundle` with
`destination: artifact-only`, and upload that signed `.aab` in Play Console.

After enabling the Google Play Developer API and granting the service account
access to Wegert, later runs may select `internal-track`. CI has no production
target; promote a tested internal build in Play Console.

Each run needs an unused, increasing integer `version_code`. `version_name` is
the user-visible release label.
