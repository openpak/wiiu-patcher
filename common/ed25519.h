// Ed25519 signature verification (RFC 8032), after TweetNaCl (public domain).
// Verify only; the signing key never leaves the owner's machine.
#pragma once
#include <cstddef>
#include <cstdint>

bool ed25519_verify(const uint8_t sig[64], const uint8_t *msg, size_t len, const uint8_t pub[32]);
