# Plasticity Electron security

Windows x64, based on Electron 42.11.6. `plasticity_secure` is a static-library
target in this repository; the Node, V8 and Chromium changes use Electron's
existing dependency patch lists.

## Code to review

| Location | Responsibility |
| --- | --- |
| `src/runtime.cc` and `shell/common/node_bindings.cc` | Start the main process from `resources/plasticity.jsc.pack`. |
| `patches/v8/plasticity_decrypt_jsc_inside_v8.patch` | Verify and decrypt JSC inside `CodeSerializer::DeserializeEncrypted`, deserialize it, and wipe the temporary plaintext. |
| `patches/node/plasticity_create_module_arguments_for_encrypted_jsc.patch` | Route `vm.Script` cached data through the protected loader; reject cached data from other application APIs; verify and initialize the private addon. |
| `shell/renderer/preload_utils.cc` and `patches/chromium/plasticity_disable_webpage_code_cache.patch` | Disable preload and webpage bytecode-cache consumption. |
| `shell/app/electron_main_win.cc` and `filenames.gni` | Remove run-as-Node execution and its implementation from the Windows build. |

Node/Electron's compiled-in caches remain available. The DuplicateHandle
compatibility patch is retained from the tested runtime for Themida compatibility.

## Build and virtualize

1. Initialize with `e init electron42 --import release --remote-build none` and
   use this fork with its listed dependency patches.
2. Set the selected output directory's `args.gn` to
   `import("//electron/build/args/plasticity.gn")`. This retains Release settings,
   pointer compression, `v8_enable_sandbox=false`, and `node_use_node_snapshot=false`.
3. Generate `src/key_config.h` using `tools/generate-key-header.mjs` with
   `--verification-key PUBLIC.pem --wrap-key WRAP.bin --runtime-id HEX_ID`
   and `--output electron/plasticity_secure/src/key_config.h`. Keep the generated
   header, wrapping key, and private signing key outside Git. Reuse these inputs
   when packaging assets for the resulting runtime.
4. Build using `e build`. Build the `plasticity_jsc_compiler` target with the same
   configuration for bytecode generation; it shares the runtime's V8/Node objects
   and cache identity. This compiler is a build tool, separate from the application.
5. Apply Code Virtualizer to the linked `electron.exe`. Regions select TIGER
   WHITE in `src/virtualizer.h`. Keep automatic ANSI/Unicode string encryption
   disabled because it misidentifies binary crypto constants.

## Package each Plasticity build

1. Finalize protection and platform signing of `pk.node`, then run
   `node tools/sign-addon.mjs ADDON PRIVATE.pem`. Package the addon and its `.sig`
   in `resources/app.asar.unpacked/.webpack/renderer/`. Its production initializer
   is `PlasticityInitialize_v1`, with the matching host challenge protocol.
2. Compile JSC with the matching compiler. `tools/seal-jsc-directory.mjs` seals
   renderer JSC and writes its `vm.Script` loader. `tools/create-asset-pack.mjs`
   creates the main pack at `resources/plasticity.jsc.pack`.
3. Both tools use the runtime's Electron/V8 versions, platform `windows`,
   architecture `x64`, runtime ID, wrapping key and signing key. See their command
   arguments in the source. Package the resulting assets and ASAR alongside the
   protected executable; application asset changes do not require relinking it.

Packs use AEGIS-128L for the payload and AES-256-GCM for the per-pack key.
Ed25519 signs SHA-256(header and ciphertext); addon signatures cover SHA-256 of
the final file. Signature decisions, key unwrapping and authentication remain
inside Virtualizer regions. Bulk hashing, decryption and buffer operations stay
native. SHA-2 assembly and its regeneration instructions are under `third_party/sha2`.
