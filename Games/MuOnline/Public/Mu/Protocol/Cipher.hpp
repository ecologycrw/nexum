#pragma once

// The encryption of the 106.3 protocol, in the role of the client: SimpleModulus with a counter in each
// direction, Xor32 on everything the client sends and Xor3 on the fields of the login. The keys are public
// constants, so this is obfuscation, not security. Everything here comes from the protocol spec of the
// knowledge center (docs/specs/protocolo/protocolo-106-3-m2.md, sections 4.1 to 4.4) and from the finding
// 2026-11-21-cifra-simplemodulus-xor32-xor3.md, which also has the test vectors.
//
// The order when the client sends (4.1): the plain message, Xor3 on some fields while it is built, Xor32 on
// the whole message, then SimpleModulus only on C3 and C4, which inserts the counter. The server only uses
// SimpleModulus.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "Nexum/Core/Result.hpp"

namespace mu::protocol {

// The three vectors of 4 numbers of a SimpleModulus key: the modulus, the xor and the multiplier (E to
// encrypt, D to decrypt; D is the modular inverse of E).
struct SimpleModulusKey {
    std::array<std::uint32_t, 4> modulus;
    std::array<std::uint32_t, 4> xorKey;
    std::array<std::uint32_t, 4> multiplier;
};

// The keys of section 4.2. The client encrypts with the first and decrypts with the second; the other two are
// the server's, and the Nexum uses them only in tests and in the test server.
inline constexpr SimpleModulusKey kClientToServerEncryptKey{
    {128079, 164742, 70235, 106898}, {48413, 46165, 15171, 37433}, {23489, 11911, 19816, 13647}};
inline constexpr SimpleModulusKey kServerToClientDecryptKey{
    {73326, 109989, 98843, 171058}, {62004, 64409, 35374, 64599}, {18035, 30340, 24701, 11141}};
inline constexpr SimpleModulusKey kClientToServerDecryptKey{
    {128079, 164742, 70235, 106898}, {48413, 46165, 15171, 37433}, {31544, 2047, 57011, 10183}};
inline constexpr SimpleModulusKey kServerToClientEncryptKey{
    {73326, 109989, 98843, 171058}, {62004, 64409, 35374, 64599}, {13169, 19036, 35482, 29587}};

// A block: 8 plain bytes become 11 on the wire (section 4.2).
inline constexpr std::size_t kPlainBlockSize = 8;
inline constexpr std::size_t kEncryptedBlockSize = 11;

// Encrypts one block. `realBytes` (1 to 8) is how many of the 8 bytes are data; the rest must be zero.
void EncryptBlock(std::span<const std::byte, kPlainBlockSize> plain, std::size_t realBytes,
                  const SimpleModulusKey& key, std::span<std::byte, kEncryptedBlockSize> encrypted) noexcept;

// Decrypts one block into 8 bytes and gives how many of them are data. A size larger than 8 or a wrong
// checksum is an error: the connection must close, with no attempt to resynchronize.
[[nodiscard]] nexum::core::Result<std::size_t>
DecryptBlock(std::span<const std::byte, kEncryptedBlockSize> encrypted, const SimpleModulusKey& key,
             std::span<std::byte, kPlainBlockSize> plain);

// The size on the wire of a plain C3 or C4: the header, then 11 bytes for each 8 bytes of the counter
// followed by the message from its code on (section 4.2).
[[nodiscard]] std::size_t EncryptedLength(std::size_t headerSize, std::size_t plainLength) noexcept;

// One direction of SimpleModulus, with its counter: it starts at 0 in each connection, goes from 255 back to
// 0, and moves only after a whole C3 or C4. C1 and C2 pass unchanged and do not use the counter.
class PacketEncryptor {
public:
    explicit PacketEncryptor(const SimpleModulusKey& key) noexcept : m_key(key) {}

    // Writes the message as it goes on the wire into `wire`, which needs EncryptedLength bytes for a C3 or C4
    // and the size of the message for a C1 or C2, and gives the size written. A C3 larger than 185 plain
    // bytes would not fit in its length byte on the wire, and is refused.
    [[nodiscard]] nexum::core::Result<std::size_t> Encrypt(std::span<const std::byte> message,
                                                           std::span<std::byte> wire);

    [[nodiscard]] std::uint8_t Counter() const noexcept { return m_counter; }

private:
    SimpleModulusKey m_key;
    std::uint8_t m_counter = 0;
};

class PacketDecryptor {
public:
    explicit PacketDecryptor(const SimpleModulusKey& key) noexcept : m_key(key) {}

    // Writes the plain message into `plain`, which needs the size of the wire message, and gives the size
    // written. A size that is not a multiple of 11, a broken block or a wrong counter is an error: the
    // connection closes.
    [[nodiscard]] nexum::core::Result<std::size_t> Decrypt(std::span<const std::byte> wire,
                                                           std::span<std::byte> plain);

    [[nodiscard]] std::uint8_t ExpectedCounter() const noexcept { return m_counter; }

private:
    SimpleModulusKey m_key;
    std::uint8_t m_counter = 0;
};

// Xor32 (section 4.3), in place: from the byte after the code to the end, each byte with the byte before it
// and the key at its position in the message. The header and the code stay in the clear.
void Xor32Encrypt(std::span<std::byte> message) noexcept;
void Xor32Decrypt(std::span<std::byte> message) noexcept;

// Xor3 (section 4.4), in place, on a whole field: each byte with the key at its position in the field.
void Xor3(std::span<std::byte> field) noexcept;

} // namespace mu::protocol
