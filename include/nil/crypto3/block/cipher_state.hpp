//---------------------------------------------------------------------------//
// Copyright (c) 2018-2019 Nil Foundation AG
// Copyright (c) 2018-2019 Mikhail Komarov <nemo@nilfoundation.org>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#ifndef CRYPTO3_CIPHER_STATE_PREPROCESSOR_HPP
#define CRYPTO3_CIPHER_STATE_PREPROCESSOR_HPP

#include <boost/accumulators/framework/accumulator_set.hpp>
#include <boost/accumulators/framework/features.hpp>

#include <nil/crypto3/block/accumulators/block.hpp>

namespace nil {
    namespace crypto3 {
        namespace block {
            /*!
             * @brief Cipher state managing container
             *
             * Meets the requirements of CipherStateContainer, ConceptContainer, SequenceContainer, Container
             *
             * @tparam Mode Cipher state preprocessing mode type (e.g. isomorphic_encryption_mode<aes128>)
             * @tparam Endian
             * @tparam ValueBits
             * @tparam LengthBits
             */
            template<typename ProcessingMode> using block_accumulator = boost::accumulators::accumulator_set<
                    block::digest<ProcessingMode::input_block_bits>,
                    boost::accumulators::features<accumulators::tag::block<ProcessingMode>>>;
        }
    }
}

#endif //CRYPTO3_CIPHER_STATE_PREPROCESSOR_HPP
