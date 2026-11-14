# Woditor for iOS

Native iOS build of Browser Woditor (WOLF RPG) with an on-screen gamepad.

## What is here
- `ios/engine-prebuilt/` — the engine, prebuilt as a static library (`libwoditor_engine.a`) plus its headers.
- `host/`, `runtime/` — the engine host (files, graphics, sound, input) and the wasm2c runtime.
- `platform/ios/` — the iOS app: window, OpenGL ES, audio, touch/keyboard/controllers, gamepad (`pad/`).
- `ios/CMakeLists.txt` — Xcode project; `.github/workflows/ios.yml` — the build.

## How to build
Actions → **iOS IPA** → **Run workflow**. Optionally set a game: a zip name from the `games` release
or a direct zip URL. Download the `.ipa` from the run's **Artifacts** and install it with Sideloadly or AltStore.

Without a bundled game, put the game files into **Files → On My iPhone → Woditor → game**.
