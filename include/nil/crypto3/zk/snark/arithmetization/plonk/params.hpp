//---------------------------------------------------------------------------//
// Copyright (c) 2022 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2022 Nikita Kaskov <nbering@nil.foundation>
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//---------------------------------------------------------------------------//

#ifndef CRYPTO3_ZK_PLONK_ARITHMETIZATION_PARAMS_HPP
#define CRYPTO3_ZK_PLONK_ARITHMETIZATION_PARAMS_HPP

namespace nil {
    namespace crypto3 {
        namespace zk {
            namespace snark {

                template<std::size_t _WitnessColumns, std::size_t _PublicInputColumns,
                    std::size_t _ConstantColumns, std::size_t _SelectorColumns>
                struct plonk_arithmetization_params {
                    constexpr static const std::size_t WitnessColumns = _WitnessColumns;
                    constexpr static const std::size_t PublicInputColumns = _PublicInputColumns;
                    constexpr static const std::size_t ConstantColumns = _ConstantColumns;
                    constexpr static const std::size_t SelectorColumns = _SelectorColumns;
                };

#ifdef ZK_RUNTIME_CIRCUIT_DEFINITION

                struct plonk_arithmetization_params {
                };
#endif
            }    // namespace snark
        }        // namespace zk
    }            // namespace crypto3
}    // namespace nil

#endif    // CRYPTO3_ZK_PLONK_ARITHMETIZATION_PARAMS_HPP