import { createHash, createPrivateKey, sign } from 'node:crypto';
import { readFileSync, writeFileSync } from 'node:fs';

const [addon, keyFile] = process.argv.slice(2);
if (!addon || !keyFile)
  throw new Error('usage: sign-addon.mjs ADDON SIGNING_KEY');
const key = createPrivateKey(readFileSync(keyFile));
if (key.asymmetricKeyType !== 'ed25519')
  throw new Error('signing key must be Ed25519');
const digest = createHash('sha256').update(readFileSync(addon)).digest();
writeFileSync(`${addon}.sig`, sign(null, digest, key));
