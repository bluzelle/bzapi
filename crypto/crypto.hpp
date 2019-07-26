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

#pragma once

#include <crypto/crypto_base.hpp>
#include <proto/bluzelle.pb.h>
#include <openssl/evp.h>
#include <openssl/ec.h>

namespace bzapi
{
    class crypto : public crypto_base
    {
    public:

        crypto(const std::string& private_key);

        bool sign(bzn_envelope& msg) override;

        bool verify(const bzn_envelope& msg) override;

    private:

        using EC_KEY_ptr_t = std::unique_ptr<EC_KEY, decltype(&::EC_KEY_free)>;
        using EVP_PKEY_ptr_t = std::unique_ptr<EVP_PKEY, decltype(&::EVP_PKEY_free)>;
        using BIO_ptr_t = std::unique_ptr<BIO, decltype(&::BIO_free)>;
        using EVP_MD_CTX_ptr_t = std::unique_ptr<EVP_MD_CTX, decltype(&::EVP_MD_CTX_free)>;

        void log_openssl_errors();

        const std::string& extract_payload(const bzn_envelope& msg);

        const std::string deterministic_serialize(const bzn_envelope& msg);

        bool load_private_key(const std::string& key);

        EVP_PKEY_ptr_t private_key_EVP = EVP_PKEY_ptr_t(nullptr, &EVP_PKEY_free);
    };
}

