//
// Copyright (C) 2019 Bluzelle
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <include/bluzelle.hpp>
#include <crypto/crypto.hpp>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/crypto.h>


using namespace bzapi;

namespace
{
    const std::string PEM_PREFIX = "-----BEGIN PUBLIC KEY-----\n";
    const std::string PEM_SUFFIX = "\n-----END PUBLIC KEY-----\n";
    const std::string PRIV_KEY_PREFIX{"-----BEGIN EC PRIVATE KEY-----\n"};
    const std::string PRIV_KEY_SUFFIX{"\n-----END EC PRIVATE KEY-----"};
}

crypto::crypto(const std::string& private_key)
{
    LOG(info) << "Using " << SSLeay_version(SSLEAY_VERSION);
    this->load_private_key(private_key);
}

const std::string&
crypto::extract_payload(const bzn_envelope& msg)
{
    switch (msg.payload_case())
    {
        case bzn_envelope::kDatabaseMsg :
        {
            return msg.database_msg();
        }
        case bzn_envelope::kDatabaseResponse :
        {
            return msg.database_response();
        }
        case bzn_envelope::kStatusRequest:
        {
            return msg.status_request();
        }
        case bzn_envelope::kStatusResponse:
        {
            return msg.status_response();
        }
        default :
        {
            throw std::runtime_error(
                    "Crypto does not know how to handle a message with type " + std::to_string(msg.payload_case()));
        }
    }
}

const std::string
crypto::deterministic_serialize(const bzn_envelope& msg)
{
    // I don't like hand-rolling this, but doing it here lets us do it in one place, while avoiding implementing
    // it for every message type

    std::vector<std::string> tokens = {msg.sender(), std::to_string(msg.payload_case()), this->extract_payload(msg), std::to_string(msg.timestamp())};

    // this construction defeats an attack where the adversary blurs the lines between the fields. if we simply
    // concatenate the fields, then consider the two messages
    //     {sender: "foo", payload: "bar"}
    //     {sender: "foobar", payload: ""}
    // they may have the same serialization - and therefore the same signature.
    std::string result = "";
    for (const auto& token : tokens)
    {
        result += (std::to_string(token.length()) + "|" + token);
    }

    return result;
}

bool
crypto::verify(const bzn_envelope& msg)
{
    BIO_ptr_t bio(BIO_new(BIO_s_mem()), &BIO_free);
    EC_KEY_ptr_t pubkey(nullptr, &EC_KEY_free);
    EVP_PKEY_ptr_t key(EVP_PKEY_new(), &EVP_PKEY_free);
    EVP_MD_CTX_ptr_t context(EVP_MD_CTX_create(), &EVP_MD_CTX_free);

    if (!bio || !key || !context)
    {
        LOG(error) << "failed to allocate memory for signature verification";
        return false;
    }

    const auto msg_text = this->deterministic_serialize(msg);

    // In openssl 1.0.1 (but not newer versions), EVP_DigestVerifyFinal strangely expects the signature as
    // a non-const pointer.
    std::string signature = msg.signature();
    char* sig_ptr = signature.data();

    bool result =
            // Reconstruct the PEM file in memory (this is awkward, but it avoids dealing with EC specifics)
            (0 < BIO_write(bio.get(), PEM_PREFIX.c_str(), PEM_PREFIX.length()))
            && (0 < BIO_write(bio.get(), msg.sender().c_str(), msg.sender().length()))
            && (0 < BIO_write(bio.get(), PEM_SUFFIX.c_str(), PEM_SUFFIX.length()))

            // Parse the PEM string to get the public key the message is allegedly from
            && (pubkey = EC_KEY_ptr_t(PEM_read_bio_EC_PUBKEY(bio.get(), NULL, NULL, NULL), &EC_KEY_free))
            && (1 == EC_KEY_check_key(pubkey.get()))
            && (1 == EVP_PKEY_set1_EC_KEY(key.get(), pubkey.get()))

            // Perform the signature validation
            && (1 == EVP_DigestVerifyInit(context.get(), NULL, EVP_sha256(), NULL, key.get()))
            && (1 == EVP_DigestVerifyUpdate(context.get(), msg_text.c_str(), msg_text.length()))
            && (1 == EVP_DigestVerifyFinal(context.get(), reinterpret_cast<unsigned char*>(sig_ptr), msg.signature().length()));

    /* Any errors here can be attributed to a bad (potentially malicious) incoming message, and we we should not
     * pollute our own logs with them (but we still have to clear the error state)
     */
    ERR_clear_error();

    return result;
}

bool
crypto::sign(bzn_envelope& msg)
{
    const auto msg_text = this->deterministic_serialize(msg);

    EVP_MD_CTX_ptr_t context(EVP_MD_CTX_create(), &EVP_MD_CTX_free);
    size_t signature_length = 0;

    bool result =
            (bool) (context)
            && (1 == EVP_DigestSignInit(context.get(), NULL, EVP_sha256(), NULL, this->private_key_EVP.get()))
            && (1 == EVP_DigestSignUpdate(context.get(), msg_text.c_str(), msg_text.length()))
            && (1 == EVP_DigestSignFinal(context.get(), NULL, &signature_length));

    auto deleter = [](unsigned char* ptr){OPENSSL_free(ptr);};
    std::unique_ptr<unsigned char, decltype(deleter)> signature((unsigned char*) OPENSSL_malloc(sizeof(unsigned char) * signature_length), deleter);

    result &=
            (bool) (signature)
            && (1 == EVP_DigestSignFinal(context.get(), signature.get(), &signature_length));

    if (result)
    {
        msg.set_signature(signature.get(), signature_length);
    }
    else
    {
        LOG(error) << "Failed to sign message with openssl error (do we have a valid private key?)";
    }

    this->log_openssl_errors();
    return result;
}

void
crypto::log_openssl_errors()
{
    unsigned long last_error;
    char buffer[120]; //openssl says this is the appropriate length

    while((last_error = ERR_get_error()))
    {
        ERR_error_string(last_error, buffer);
        std::cout << buffer << std::endl;
        LOG(error) << buffer;
    }
}

bool
crypto::load_private_key(const std::string& key)
{
    auto formatted_key = PRIV_KEY_PREFIX + key + PRIV_KEY_SUFFIX;
    BIO *bio = BIO_new_mem_buf((void *)formatted_key.c_str(), formatted_key.length());
    this->private_key_EVP = EVP_PKEY_ptr_t(PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL), EVP_PKEY_free);
    BIO_free(bio);

    if (!this->private_key_EVP)
    {
        LOG(error) << "error loading private key";
        return false;
    }

    return true;
}
