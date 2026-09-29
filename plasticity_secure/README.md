# Plasticity Secure

`plasticity_secure` is a static-library target in the Plasticity Electron fork.
The checks are shared across Windows, macOS and Linux on x64/ARM64 (Windows x64).
Windows has full application validation. The macOS port has native component
tests; full macOS/Linux Electron builds and application tests remain pending.

The release asset pack contains the source string, V8 cached data and addon signature encrypted
together with AEGIS-128L, plus the Electron, V8, platform, architecture, and
runtime identities. Ed25519 signs the SHA-256 digest of the complete header and
ciphertext. Content-key unwrapping, authenticated-decryption finalization, and JSC admission execute
inside Code Virtualizer regions. Buffer allocation and the AEGIS bulk loop stay
native.

`tools/generate-key-header.mjs` generates `src/key_config.h` from an Ed25519
public verification key and the wrapping key. Plasticity's
`tools/security/create-asset-pack.mjs` creates the signed encrypted JSC pack;
`yarn seal-jsc` seals the staging application's JSC files before ASAR packaging.
Install it as `resources/plasticity.jsc.pack` beside `resources/app.asar`, or in
`Contents/Resources` on macOS.
The runtime reads and verifies the pack in memory before decrypting the JSC.

Node verifies Ed25519(SHA-256(addon bytes)) before loading the production addon
through its private initializer at the fixed application path.
Each authenticated JSC supplies the native thread's addon signature. Seal the JSC
after the addon's final protection/signing step; no detached `.sig` is shipped.
Before calling the initializer, the loader compares the mapped file identity with
the verified file and checks the initializer belongs to that mapped image. This
is a post-load check, not prevention of a substituted library's startup code.
Windows compares mapped file IDs; macOS queries the initializer's VM-region
vnode identity; Linux compares its `/proc/self/maps` device/inode. POSIX does not
provide Windows' mandatory file-sharing restriction against concurrent writes.

Private SHA-2 uses CPU-selected OpenSSL assembly for bulk blocks, with protected
finalization and signature decisions. Both the packager and runtime use this
digest-signing scheme; all packaged JSC packs and addon signatures are regenerated.

Initialize the build with `e init electron42 --import release --remote-build none`.
Build Electron with `e build`, retaining `v8_enable_sandbox=false`. Compile JSC
with the `plasticity_jsc_compiler` target built from the same checkout and flags;
an official Electron binary can have a different V8 cache identity. Apply Code
Virtualizer to the linked executable (the Electron Framework binary on macOS).
The same protected regions emit GNU SDK markers on macOS/Linux; marker emission
does not replace running and validating the protector on the final binary.
Package the signed JSC sidecar and ASAR
after generating the renderer bundle against the final protected addon.
Keep automatic ANSI/Unicode string encryption disabled: it misidentifies binary
crypto constants and breaks the protected V8 signature verification. TIGER
virtualization remains enabled.

`patches/node/plasticity_create_module_arguments_for_encrypted_jsc.patch` owns
the private addon loader and JavaScript-facing cached-data restrictions.
`patches/v8/plasticity_decrypt_jsc_inside_v8.patch` owns authenticated in-memory
deserialization. `patches/chromium/plasticity_disable_webpage_code_cache.patch`
disables webpage bytecode caching. Ordinary Node/Electron built-in caches remain.
All three platform entrypoints remove run-as-Node startup; its implementation is
excluded from the application build on every platform.
