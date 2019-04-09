// Copyright (C) 2018 Bluzelle
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License, version 3,
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
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
        EC_KEY_ptr_t private_key_EC = EC_KEY_ptr_t(nullptr, &EC_KEY_free);

    };
}

