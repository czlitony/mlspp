#include <doctest/doctest.h>
#include <mls/crypto.h>

#include <string>

using namespace mls;

TEST_CASE("Basic HPKE")
{
  auto info = random_bytes(100);
  auto aad = random_bytes(100);
  auto original = random_bytes(100);

  for (auto suite_id : all_supported_suites) {
    auto suite = CipherSuite{ suite_id };
    auto s = bytes{ 0, 1, 2, 3 };

    auto x = HPKEPrivateKey::generate(suite);
    auto y = HPKEPrivateKey::derive(suite, { 0, 1, 2, 3 });

    REQUIRE(x == x);
    REQUIRE(y == y);
    REQUIRE(x != y);

    auto gX = x.public_key;
    auto gY = y.public_key;
    REQUIRE(gX == gX);
    REQUIRE(gY == gY);
    REQUIRE(gX != gY);

    auto encrypted = gX.encrypt(suite, info, aad, original);
    auto decrypted = x.decrypt(suite, info, aad, encrypted);

    REQUIRE(original == decrypted);
  }
}

TEST_CASE("HPKE Key Serialization")
{
  for (auto suite_id : all_supported_suites) {
    auto suite = CipherSuite{ suite_id };
    auto x = HPKEPrivateKey::derive(suite, { 0, 1, 2, 3 });
    auto gX = x.public_key;

    HPKEPublicKey parsed{ gX.data };
    REQUIRE(parsed == gX);

    auto marshaled = tls::marshal(gX);
    auto gX2 = tls::get<HPKEPublicKey>(marshaled);
    REQUIRE(gX2 == gX);
  }
}

TEST_CASE("Basic Signature")
{
  for (auto suite_id : all_supported_suites) {
    auto suite = CipherSuite{ suite_id };
    auto a = SignaturePrivateKey::generate(suite);
    auto b = SignaturePrivateKey::generate(suite);

    REQUIRE(a == a);
    REQUIRE(b == b);
    REQUIRE(a != b);

    REQUIRE(a.public_key == a.public_key);
    REQUIRE(b.public_key == b.public_key);
    REQUIRE(a.public_key != b.public_key);

    auto label = from_ascii("label");
    auto message = from_hex("01020304");
    auto signature = a.sign(suite, label, message);

    REQUIRE(a.public_key.verify(suite, label, message, signature));
  }
}

TEST_CASE("Signature Key Serializion")
{
  for (auto suite_id : all_supported_suites) {
    auto suite = CipherSuite{ suite_id };
    auto x = SignaturePrivateKey::generate(suite);
    auto gX = x.public_key;

    SignaturePublicKey parsed{ gX.data };
    REQUIRE(parsed == gX);

    auto gX2 = tls::get<SignaturePublicKey>(tls::marshal(gX));
    REQUIRE(gX2 == gX);
  }
}
