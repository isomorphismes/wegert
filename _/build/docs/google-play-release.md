# Google Play release

Google Play is a separate distribution/signing boundary for the same Android
application identity used elsewhere in this repository.

- package ID: `org.isomorphisms.wegert`
- release version: `fdroid/release.properties`
- Android build inputs: the same pinned values in `fdroid/release.properties`
- Play upload signer: private and separate from the repository's public test key
- F-Droid signer: controlled by F-Droid and separate from both of the above

The Play workflow does not invent its own version code or version name. Update
the source-controlled release identity once, keep
`app/build.gradle.kts`, `fdroid/release.properties`, and the F-Droid recipe
consistent, and let `fdroid/verify-metadata.sh` enforce that shared boundary.

## Validation

Every pull request that changes the Play/build inputs creates a disposable
PKCS12 key and builds a signed release AAB. That validates the release signing
configuration without exposing or requiring the real upload key. CI also verifies that full native debug symbols for every packaged ABI are embedded in the AAB and retains that signed bundle.

The disposable key is never a release identity and must not be installed or
enrolled in Play.

## Protected environment

Create a protected GitHub environment named `google-play` with these secrets:

- `ANDROID_UPLOAD_KEYSTORE_BASE64`
- `ANDROID_UPLOAD_KEYSTORE_PASSWORD`
- `ANDROID_UPLOAD_KEY_ALIAS`
- `ANDROID_UPLOAD_KEY_PASSWORD`
- `GOOGLE_PLAY_SERVICE_ACCOUNT_JSON`

The Android secrets describe the private Play **upload** key. Enroll the app in
Play App Signing so Google's app-signing key remains a separate device-delivery
identity.

The service-account secret is required only for API upload to the internal
track. An `artifact-only` run builds and retains the signed AAB without using
the publishing API.

## First upload

The publishing API expects the package to exist already in Play Console. For a
new listing:

1. Create the app in Play Console with package ID
   `org.isomorphisms.wegert`.
2. Complete the required store/policy declarations.
3. Run **Google Play app bundle** with `destination: artifact-only`.
4. Upload that signed AAB manually for the first release.
5. Enable the Google Play Developer API and grant the service account access to
   this app.

Later workflow runs may choose `internal-track`. The workflow contains no
production-track target; promotion beyond internal testing remains an explicit
Play Console action.

## Relationship to Triple-T/F-Droid metadata

The canonical upstream metadata tree remains `app/src/main/play`. The Play
workflow currently consumes its English release note for the internal-track
"what's new" text, but does not automatically overwrite the Play store listing.
That avoids turning F-Droid-specific naming or listing decisions into Play
Console changes by accident.
