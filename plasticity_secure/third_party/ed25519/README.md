Sources copied from Plasticity's `vendor/keygen/ed25519`, originally
https://github.com/orlp/ed25519. Only the verification arithmetic and SHA-512
are built here. Verification and point encoding execute directly inside the
protected `CodeSerializer::DeserializeEncrypted` region in V8 and the private
Node addon loader.

Local changes: `ge_tobytes` is expanded into its caller, as in Plasticity's
`4a0262b15`. `sha512_init` and the output stage of
`sha512_final` use the Electron fork's Virtualizer markers and remain out of
line. Bulk hashing and scalar multiplication remain native.
