#include <mls/core_types.h>
#include <mls/crypto.h>
#include <mls/log.h>
#include <mls/messages.h>

#include <string>

using hpke::AEAD;      // NOLINT(misc-unused-using-decls)
using hpke::Digest;    // NOLINT(misc-unused-using-decls)
using hpke::HPKE;      // NOLINT(misc-unused-using-decls)
using hpke::KDF;       // NOLINT(misc-unused-using-decls)
using hpke::KEM;       // NOLINT(misc-unused-using-decls)
using hpke::Signature; // NOLINT(misc-unused-using-decls)

using mls::log::Log;
static const auto log_mod = "crypto"s;

namespace mls {

SignatureScheme
tls_signature_scheme(Signature::ID id)
{
  switch (id) {
    case Signature::ID::P256_SHA256:
      return SignatureScheme::ecdsa_secp256r1_sha256;
    case Signature::ID::P384_SHA384:
      return SignatureScheme::ecdsa_secp384r1_sha384;
    case Signature::ID::P521_SHA512:
      return SignatureScheme::ecdsa_secp521r1_sha512;
    case Signature::ID::Ed25519:
      return SignatureScheme::ed25519;
    case Signature::ID::Ed448:
      return SignatureScheme::ed448;
    case Signature::ID::RSA_SHA256:
      return SignatureScheme::rsa_pkcs1_sha256;
    default:
      throw InvalidParameterError("Unsupported algorithm");
  }
}

///
/// CipherSuites and details
///

CipherSuite::CipherSuite()
  : id(ID::unknown)
{
}

CipherSuite::CipherSuite(ID id_in)
  : id(id_in)
{
}

SignatureScheme
CipherSuite::signature_scheme() const
{
  switch (id) {
    case ID::X25519_AES128GCM_SHA256_Ed25519:
    case ID::X25519_CHACHA20POLY1305_SHA256_Ed25519:
      return SignatureScheme::ed25519;
    case ID::P256_AES128GCM_SHA256_P256:
      return SignatureScheme::ecdsa_secp256r1_sha256;
    case ID::X448_AES256GCM_SHA512_Ed448:
    case ID::X448_CHACHA20POLY1305_SHA512_Ed448:
      return SignatureScheme::ed448;
    case ID::P521_AES256GCM_SHA512_P521:
      return SignatureScheme::ecdsa_secp521r1_sha512;
    default:
      throw InvalidParameterError("Unsupported algorithm");
  }
}

const CipherSuite::Ciphers&
CipherSuite::get() const
{
  static const auto ciphers_X25519_AES128GCM_SHA256_Ed25519 =
    CipherSuite::Ciphers{
      HPKE(KEM::ID::DHKEM_X25519_SHA256,
           KDF::ID::HKDF_SHA256,
           AEAD::ID::AES_128_GCM),
      Digest::get<Digest::ID::SHA256>(),
      Signature::get<Signature::ID::Ed25519>(),
    };

  static const auto ciphers_P256_AES128GCM_SHA256_P256 = CipherSuite::Ciphers{
    HPKE(
      KEM::ID::DHKEM_P256_SHA256, KDF::ID::HKDF_SHA256, AEAD::ID::AES_128_GCM),
    Digest::get<Digest::ID::SHA256>(),
    Signature::get<Signature::ID::P256_SHA256>(),
  };

  static const auto ciphers_X25519_CHACHA20POLY1305_SHA256_Ed25519 =
    CipherSuite::Ciphers{
      HPKE(KEM::ID::DHKEM_X25519_SHA256,
           KDF::ID::HKDF_SHA256,
           AEAD::ID::CHACHA20_POLY1305),
      Digest::get<Digest::ID::SHA256>(),
      Signature::get<Signature::ID::Ed25519>(),
    };

  static const auto ciphers_X448_AES256GCM_SHA512_Ed448 = CipherSuite::Ciphers{
    HPKE(
      KEM::ID::DHKEM_X448_SHA512, KDF::ID::HKDF_SHA512, AEAD::ID::AES_256_GCM),
    Digest::get<Digest::ID::SHA512>(),
    Signature::get<Signature::ID::Ed448>(),
  };

  static const auto ciphers_P521_AES256GCM_SHA512_P521 = CipherSuite::Ciphers{
    HPKE(
      KEM::ID::DHKEM_P521_SHA512, KDF::ID::HKDF_SHA512, AEAD::ID::AES_256_GCM),
    Digest::get<Digest::ID::SHA512>(),
    Signature::get<Signature::ID::P521_SHA512>(),
  };

  static const auto ciphers_X448_CHACHA20POLY1305_SHA512_Ed448 =
    CipherSuite::Ciphers{
      HPKE(KEM::ID::DHKEM_X448_SHA512,
           KDF::ID::HKDF_SHA512,
           AEAD::ID::CHACHA20_POLY1305),
      Digest::get<Digest::ID::SHA512>(),
      Signature::get<Signature::ID::Ed448>(),
    };

  switch (id) {
    case ID::unknown:
      throw InvalidParameterError("Uninitialized ciphersuite");

    case ID::X25519_AES128GCM_SHA256_Ed25519:
      return ciphers_X25519_AES128GCM_SHA256_Ed25519;

    case ID::P256_AES128GCM_SHA256_P256:
      return ciphers_P256_AES128GCM_SHA256_P256;

    case ID::X25519_CHACHA20POLY1305_SHA256_Ed25519:
      return ciphers_X25519_CHACHA20POLY1305_SHA256_Ed25519;

    case ID::X448_AES256GCM_SHA512_Ed448:
      return ciphers_X448_AES256GCM_SHA512_Ed448;

    case ID::P521_AES256GCM_SHA512_P521:
      return ciphers_P521_AES256GCM_SHA512_P521;

    case ID::X448_CHACHA20POLY1305_SHA512_Ed448:
      return ciphers_X448_CHACHA20POLY1305_SHA512_Ed448;

    default:
      throw InvalidParameterError("Unsupported ciphersuite");
  }
}

struct HKDFLabel
{
  uint16_t length;
  bytes label;
  bytes context;

  TLS_SERIALIZABLE(length, label, context)
};

bytes
CipherSuite::expand_with_label(const bytes& secret,
                               const std::string& label,
                               const bytes& context,
                               size_t length) const
{
  auto mls_label = from_ascii(std::string("mls10 ") + label);
  auto length16 = static_cast<uint16_t>(length);
  auto label_bytes = tls::marshal(HKDFLabel{ length16, mls_label, context });
  auto derived = get().hpke.kdf.expand(secret, label_bytes, length);

  Log::crypto(log_mod, "=== ExpandWithLabel ===");
  Log::crypto(log_mod, "  secret ", to_hex(secret));
  Log::crypto(log_mod, "  label  ", to_hex(label_bytes));
  Log::crypto(log_mod, "  length ", length);

  return derived;
}

bytes
CipherSuite::derive_secret(const bytes& secret, const std::string& label) const
{
  Log::crypto(log_mod, "=== DeriveSecret ===");
  return expand_with_label(secret, label, {}, secret_size());
}

const std::array<CipherSuite::ID, 6> all_supported_suites = {
  CipherSuite::ID::X25519_AES128GCM_SHA256_Ed25519,
  CipherSuite::ID::P256_AES128GCM_SHA256_P256,
  CipherSuite::ID::X25519_CHACHA20POLY1305_SHA256_Ed25519,
  CipherSuite::ID::X448_AES256GCM_SHA512_Ed448,
  CipherSuite::ID::P521_AES256GCM_SHA512_P521,
  CipherSuite::ID::X448_CHACHA20POLY1305_SHA512_Ed448,
};

// MakeKeyPackageRef(value) = KDF.expand(
//   KDF.extract("", value), "MLS 1.0 KeyPackage Reference", 16)
template<>
const bytes&
CipherSuite::reference_label<KeyPackage>()
{
  static const auto label = from_ascii("MLS 1.0 KeyPackage Reference");
  return label;
}

// MakeProposalRef(value) = KDF.expand(
//   KDF.extract("", value), "MLS 1.0 Proposal Reference", 16)
//
// Even though the label says "Proposal", we actually hash the entire enclosing
// MLSAuthenticatedContent object.
template<>
const bytes&
CipherSuite::reference_label<MLSAuthenticatedContent>()
{
  static const auto label = from_ascii("MLS 1.0 Proposal Reference");
  return label;
}

///
/// HPKEPublicKey and HPKEPrivateKey
///
HPKECiphertext
HPKEPublicKey::encrypt(CipherSuite suite,
                       const bytes& info,
                       const bytes& aad,
                       const bytes& pt) const
{
  auto pkR = suite.hpke().kem.deserialize(data);
  auto [enc, ctx] = suite.hpke().setup_base_s(*pkR, info);
  auto ct = ctx.seal(aad, pt);
  return HPKECiphertext{ enc, ct };
}

std::tuple<bytes, bytes>
HPKEPublicKey::do_export(CipherSuite suite,
                         const bytes& info,
                         const std::string& label,
                         size_t size) const
{
  auto label_data = from_ascii(label);
  auto pkR = suite.hpke().kem.deserialize(data);
  auto [enc, ctx] = suite.hpke().setup_base_s(*pkR, info);
  auto exported = ctx.do_export(label_data, size);
  return std::make_tuple(enc, exported);
}

HPKEPrivateKey
HPKEPrivateKey::generate(CipherSuite suite)
{
  auto priv = suite.hpke().kem.generate_key_pair();
  auto priv_data = suite.hpke().kem.serialize_private(*priv);
  auto pub = priv->public_key();
  auto pub_data = suite.hpke().kem.serialize(*pub);
  return { priv_data, pub_data };
}

HPKEPrivateKey
HPKEPrivateKey::parse(CipherSuite suite, const bytes& data)
{
  auto priv = suite.hpke().kem.deserialize_private(data);
  auto pub = priv->public_key();
  auto pub_data = suite.hpke().kem.serialize(*pub);
  return { data, pub_data };
}

HPKEPrivateKey
HPKEPrivateKey::derive(CipherSuite suite, const bytes& secret)
{
  auto priv = suite.hpke().kem.derive_key_pair(secret);
  auto priv_data = suite.hpke().kem.serialize_private(*priv);
  auto pub = priv->public_key();
  auto pub_data = suite.hpke().kem.serialize(*pub);
  return { priv_data, pub_data };
}

bytes
HPKEPrivateKey::decrypt(CipherSuite suite,
                        const bytes& info,
                        const bytes& aad,
                        const HPKECiphertext& ct) const
{
  auto skR = suite.hpke().kem.deserialize_private(data);
  auto ctx = suite.hpke().setup_base_r(ct.kem_output, *skR, info);
  auto pt = ctx.open(aad, ct.ciphertext);
  if (!pt) {
    throw InvalidParameterError("HPKE decryption failure");
  }

  return opt::get(pt);
}

bytes
HPKEPrivateKey::do_export(CipherSuite suite,
                          const bytes& info,
                          const bytes& kem_output,
                          const std::string& label,
                          size_t size) const
{
  auto label_data = from_ascii(label);
  auto skR = suite.hpke().kem.deserialize_private(data);
  auto ctx = suite.hpke().setup_base_r(kem_output, *skR, info);
  return ctx.do_export(label_data, size);
}

HPKEPrivateKey::HPKEPrivateKey(bytes priv_data, bytes pub_data)
  : data(std::move(priv_data))
  , public_key{ std::move(pub_data) }
{
}

///
/// SignaturePublicKey and SignaturePrivateKey
///

// This function produces a non-literal type, so it can't be constexpr.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SIGN_LABEL(label) from_ascii("MLS 1.0 " label)

namespace sign_label {
const bytes mls_content = SIGN_LABEL("MLSContentTBS");
const bytes leaf_node = SIGN_LABEL("LeafNodeTBS");
const bytes key_package = SIGN_LABEL("KeyPackageTBS");
const bytes group_info = SIGN_LABEL("GroupInfoTBS");
} // namespace sign_label

struct SignContent
{
  const bytes& label;
  const bytes& content;
  TLS_SERIALIZABLE(label, content)
};

bool
SignaturePublicKey::verify(const CipherSuite& suite,
                           const bytes& label,
                           const bytes& message,
                           const bytes& signature) const
{
  const auto content = tls::marshal(SignContent{ label, message });
  auto pub = suite.sig().deserialize(data);
  return suite.sig().verify(content, signature, *pub);
}

SignaturePrivateKey
SignaturePrivateKey::generate(CipherSuite suite)
{
  auto priv = suite.sig().generate_key_pair();
  auto priv_data = suite.sig().serialize_private(*priv);
  auto pub = priv->public_key();
  auto pub_data = suite.sig().serialize(*pub);
  return { priv_data, pub_data };
}

SignaturePrivateKey
SignaturePrivateKey::parse(CipherSuite suite, const bytes& data)
{
  auto priv = suite.sig().deserialize_private(data);
  auto pub = priv->public_key();
  auto pub_data = suite.sig().serialize(*pub);
  return { data, pub_data };
}

SignaturePrivateKey
SignaturePrivateKey::derive(CipherSuite suite, const bytes& secret)
{
  auto priv = suite.sig().derive_key_pair(secret);
  auto priv_data = suite.sig().serialize_private(*priv);
  auto pub = priv->public_key();
  auto pub_data = suite.sig().serialize(*pub);
  return { priv_data, pub_data };
}

bytes
SignaturePrivateKey::sign(const CipherSuite& suite,
                          const bytes& label,
                          const bytes& message) const
{
  const auto content = tls::marshal(SignContent{ label, message });
  const auto priv = suite.sig().deserialize_private(data);
  return suite.sig().sign(content, *priv);
}

SignaturePrivateKey::SignaturePrivateKey(bytes priv_data, bytes pub_data)
  : data(std::move(priv_data))
  , public_key{ std::move(pub_data) }
{
}

} // namespace mls
