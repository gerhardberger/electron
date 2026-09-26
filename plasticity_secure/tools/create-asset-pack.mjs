import {
  createCipheriv,
  createHash,
  createPrivateKey,
  randomBytes,
  sign,
} from 'node:crypto';
import { readFileSync, writeFileSync } from 'node:fs';

const headerSize = 140;
const signatureSize = 64;
const authenticatedHeaderSize = 64;

const aesSbox = Uint8Array.from([
  0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
  0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
  0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
  0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
  0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
  0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
  0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
  0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
  0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
  0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
  0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
  0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
  0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
  0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
  0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
  0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16,
]);
const shiftRows = Uint8Array.from([0, 5, 10, 15, 4, 9, 14, 3, 8, 13, 2, 7, 12, 1, 6, 11]);

function doubleByte(value) {
  return ((value << 1) ^ ((value & 0x80) === 0 ? 0 : 0x1b)) & 0xff;
}

function aesRound(output, outputOffset, input, inputOffset, key, keyOffset) {
  for (let column = 0; column < 4; ++column) {
    const offset = column * 4;
    const a = aesSbox[input[inputOffset + shiftRows[offset]]];
    const b = aesSbox[input[inputOffset + shiftRows[offset + 1]]];
    const c = aesSbox[input[inputOffset + shiftRows[offset + 2]]];
    const d = aesSbox[input[inputOffset + shiftRows[offset + 3]]];
    const all = a ^ b ^ c ^ d;
    output[outputOffset + offset] = a ^ all ^ doubleByte(a ^ b) ^ key[keyOffset + offset];
    output[outputOffset + offset + 1] = b ^ all ^ doubleByte(b ^ c) ^ key[keyOffset + offset + 1];
    output[outputOffset + offset + 2] = c ^ all ^ doubleByte(c ^ d) ^ key[keyOffset + offset + 2];
    output[outputOffset + offset + 3] = d ^ all ^ doubleByte(d ^ a) ^ key[keyOffset + offset + 3];
  }
}

function aegisUpdate(state, scratch, first, firstOffset, second, secondOffset) {
  for (let index = 7; index > 0; --index)
    aesRound(scratch, index * 16, state, (index - 1) * 16, state, index * 16);
  aesRound(scratch, 0, state, 112, state, 0);
  for (let index = 0; index < 16; ++index) {
    scratch[index] ^= first[firstOffset + index];
    scratch[64 + index] ^= second[secondOffset + index];
  }
  state.set(scratch);
}

function encryptAegis128L(plaintext, associatedData, keyMaterial, nonce) {
  const constant0 = Uint8Array.from([0x00, 0x01, 0x01, 0x02, 0x03, 0x05, 0x08, 0x0d, 0x15, 0x22, 0x37, 0x59, 0x90, 0xe9, 0x79, 0x62]);
  const constant1 = Uint8Array.from([0xdb, 0x3d, 0x18, 0x55, 0x6d, 0xc2, 0x2f, 0xf1, 0x20, 0x11, 0x31, 0x42, 0x73, 0xb5, 0x28, 0xdd]);
  const key = Buffer.from(keyMaterial);
  const state = new Uint8Array(128);
  const scratch = new Uint8Array(128);
  for (let index = 0; index < 16; ++index) {
    state[index] = key[index] ^ nonce[index];
    state[16 + index] = constant1[index];
    state[32 + index] = constant0[index];
    state[48 + index] = constant1[index];
    state[64 + index] = key[index] ^ nonce[index];
    state[80 + index] = key[index] ^ constant0[index];
    state[96 + index] = key[index] ^ constant1[index];
    state[112 + index] = key[index] ^ constant0[index];
  }
  for (let round = 0; round < 10; ++round) aegisUpdate(state, scratch, nonce, 0, key, 0);
  for (let offset = 0; offset < associatedData.length; offset += 32)
    aegisUpdate(state, scratch, associatedData, offset, associatedData, offset + 16);

  const ciphertext = Buffer.alloc(plaintext.length);
  let offset = 0;
  for (; offset + 32 <= plaintext.length; offset += 32) {
    for (let index = 0; index < 16; ++index) {
      ciphertext[offset + index] = plaintext[offset + index] ^ state[96 + index] ^ state[16 + index] ^ (state[32 + index] & state[48 + index]);
      ciphertext[offset + 16 + index] = plaintext[offset + 16 + index] ^ state[80 + index] ^ state[32 + index] ^ (state[96 + index] & state[112 + index]);
    }
    aegisUpdate(state, scratch, plaintext, offset, plaintext, offset + 16);
  }
  if (offset !== plaintext.length) {
    const tail = Buffer.alloc(32);
    plaintext.copy(tail, 0, offset);
    for (let index = 0; index < plaintext.length - offset; ++index) {
      const stream = index < 16
        ? state[96 + index] ^ state[16 + index] ^ (state[32 + index] & state[48 + index])
        : state[80 + index - 16] ^ state[32 + index - 16] ^ (state[96 + index - 16] & state[112 + index - 16]);
      ciphertext[offset + index] = tail[index] ^ stream;
    }
    aegisUpdate(state, scratch, tail, 0, tail, 16);
  }

  const lengths = Buffer.alloc(16);
  lengths.writeBigUInt64LE(BigInt(associatedData.length) * 8n, 0);
  lengths.writeBigUInt64LE(BigInt(plaintext.length) * 8n, 8);
  for (let index = 0; index < 16; ++index) lengths[index] ^= state[32 + index];
  for (let round = 0; round < 7; ++round) aegisUpdate(state, scratch, lengths, 0, lengths, 0);
  const tag = Buffer.alloc(16);
  for (let index = 0; index < 16; ++index)
    tag[index] = state[index] ^ state[16 + index] ^ state[32 + index] ^ state[48 + index] ^ state[64 + index] ^ state[80 + index] ^ state[96 + index];
  key.fill(0);
  state.fill(0);
  scratch.fill(0);
  return { ciphertext, tag };
}

function argumentsByName(values) {
  const result = new Map();
  for (let index = 0; index < values.length; index += 2) {
    const name = values[index];
    const value = values[index + 1];
    if (!name?.startsWith('--') || value === undefined)
      throw new Error('arguments must use --name value');
    const key = name.slice(2);
    if (result.has(key)) throw new Error(`duplicate --${key}`);
    result.set(key, value);
  }
  return result;
}

function required(args, name) {
  const value = args.get(name);
  if (value === undefined) throw new Error(`missing --${name}`);
  return value;
}

function version(value, parts, name) {
  const components = value.split('.');
  if (components.some(part => !/^\d+$/.test(part)))
    throw new Error(`${name} must contain ${parts} numeric components`);
  const values = components.map(part => Number.parseInt(part, 10));
  if (values.length !== parts || values.some(part => !Number.isInteger(part) || part < 0 || part > 0xffff))
    throw new Error(`${name} must contain ${parts} numeric components`);
  return values;
}

const args = argumentsByName(process.argv.slice(2));
const jsc = readFileSync(required(args, 'jsc'));
const source = readFileSync(required(args, 'source'));
const jscPayload = Buffer.concat([source, jsc]);
const [electronMajor, electronMinor, electronPatch] =
  version(required(args, 'electron-version'), 3, 'Electron version');
const [v8Major, v8Minor, v8Build, v8Patch] =
  version(required(args, 'v8-version'), 4, 'V8 version');
const runtimeId = Buffer.from(required(args, 'runtime-id'), 'hex');
const wrapKey = readFileSync(required(args, 'wrap-key'));
const privateKey = createPrivateKey(readFileSync(required(args, 'signing-key')));
if (privateKey.asymmetricKeyType !== 'ed25519') throw new Error('signing key must be Ed25519');

if (jsc.length === 0 || jsc.length > 0x7fffffff) throw new Error('invalid JSC length');
if (source.length === 0 || source.length > 0x7fffffff) throw new Error('invalid source length');
if (jscPayload.length > 0xffffffff) throw new Error('encrypted JSC is too large');
if (runtimeId.length !== 16) throw new Error('runtime id must contain 32 hexadecimal characters');
if (wrapKey.length !== 32) throw new Error('wrapping key must contain exactly 32 bytes');

const platforms = new Map([['windows', 1], ['macos', 2], ['linux', 3]]);
const architectures = new Map([['x64', 1], ['arm64', 2]]);
const platform = platforms.get(required(args, 'platform'));
const architecture = architectures.get(required(args, 'architecture'));
if (platform === undefined) throw new Error('unsupported platform');
if (architecture === undefined) throw new Error('unsupported architecture');

const header = Buffer.alloc(headerSize);
const packLength = headerSize + jscPayload.length + signatureSize;
if (packLength > 0xffffffff) throw new Error('asset pack is too large');
header.write('PLSEC001', 0, 'ascii');
header.writeUInt16LE(1, 8);
header.writeUInt16LE(headerSize, 10);
header.writeUInt32LE(packLength, 12);
header.writeUInt16LE(electronMajor, 16);
header.writeUInt16LE(electronMinor, 18);
header.writeUInt16LE(electronPatch, 20);
header.writeUInt16LE(v8Major, 22);
header.writeUInt16LE(v8Minor, 24);
header.writeUInt16LE(v8Build, 26);
header.writeUInt16LE(v8Patch, 28);
header.writeUInt8(platform, 30);
header.writeUInt8(architecture, 31);
header.writeUInt32LE(source.length, 32);
runtimeId.copy(header, 36);

const contentKey = randomBytes(16);
const jscNonce = randomBytes(16);
const { ciphertext: jscCiphertext, tag: jscTag } =
  encryptAegis128L(jscPayload, header.subarray(0, authenticatedHeaderSize), contentKey, jscNonce);
jscNonce.copy(header, 64);
jscTag.copy(header, 80);

const wrapNonce = randomBytes(12);
const wrapCipher = createCipheriv('aes-256-gcm', wrapKey, wrapNonce);
wrapCipher.setAAD(header.subarray(0, authenticatedHeaderSize));
const wrappedKey = Buffer.concat([wrapCipher.update(contentKey), wrapCipher.final()]);
const wrappedKeyTag = wrapCipher.getAuthTag();
wrapNonce.copy(header, 96);
wrappedKey.copy(header, 108);
wrappedKeyTag.copy(header, 124);

const signedData = Buffer.concat([header, jscCiphertext]);
const signature = sign(null, createHash('sha256').update(signedData).digest(), privateKey);
if (signature.length !== signatureSize) throw new Error('unexpected signature length');
const pack = Buffer.concat([signedData, signature]);
if (pack.length !== header.readUInt32LE(12)) throw new Error('asset pack length mismatch');
writeFileSync(required(args, 'output'), pack);
contentKey.fill(0);
wrapKey.fill(0);

console.log(`created ${pack.length} byte Plasticity asset pack`);
