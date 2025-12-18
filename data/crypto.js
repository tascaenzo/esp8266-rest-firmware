// crypto.js - small helpers using Web Crypto API

function hexToBytes(hex) {
  if (typeof hex !== "string") {
    throw new Error("Hex key must be a string");
  }
  const clean = hex.trim().toLowerCase();
  if (clean.length % 2 !== 0) {
    throw new Error("Hex length must be even");
  }
  const bytes = new Uint8Array(clean.length / 2);
  for (let i = 0; i < clean.length; i += 2) {
    const byte = clean.substr(i, 2);
    const value = Number.parseInt(byte, 16);
    if (Number.isNaN(value)) {
      throw new Error("Invalid hex character");
    }
    bytes[i / 2] = value;
  }
  return bytes;
}

function bytesToHex(uint8Array) {
  return Array.from(uint8Array)
    .map((b) => b.toString(16).padStart(2, "0"))
    .join("");
}

async function hmacSha256Hex(keyHex, dataString) {
  if (!globalThis.crypto || !crypto.subtle) {
    throw new Error("Web Crypto API not available");
  }
  const keyBytes = hexToBytes(keyHex);
  const encoder = new TextEncoder();
  const data = encoder.encode(dataString);
  const cryptoKey = await crypto.subtle.importKey(
    "raw",
    keyBytes,
    { name: "HMAC", hash: "SHA-256" },
    false,
    ["sign"]
  );
  const signature = await crypto.subtle.sign("HMAC", cryptoKey, data);
  return bytesToHex(new Uint8Array(signature));
}
