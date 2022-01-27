//---------------------------------------------------------------------------//
// Copyright (c) 2021 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2021 Ilias Khairullin <ilias@nil.foundation>
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

#define BOOST_TEST_MODULE crypto3_marshalling_merkle_proof_test

#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <iostream>
#include <iomanip>

#include <nil/marshalling/status_type.hpp>
#include <nil/marshalling/field_type.hpp>
#include <nil/marshalling/endianness.hpp>

#include <nil/crypto3/multiprecision/cpp_int.hpp>
#include <nil/crypto3/multiprecision/number.hpp>

#include <nil/crypto3/hash/sha2.hpp>

#include <nil/crypto3/marshalling/containers/types/merkle_proof.hpp>

template<typename TIter>
void print_byteblob(TIter iter_begin, TIter iter_end) {
    for (TIter it = iter_begin; it != iter_end; it++) {
        std::cout << std::hex << int(*it) << std::endl;
    }
}

template<typename FpCurveGroupElement>
void print_fp_curve_group_element(FpCurveGroupElement e) {
    std::cout << e.X.data << " " << e.Y.data << " " << e.Z.data << std::endl;
}

template<typename Fp2CurveGroupElement>
void print_fp2_curve_group_element(Fp2CurveGroupElement e) {
    std::cout << "(" << e.X.data[0].data << " " << e.X.data[1].data << ") (" << e.Y.data[0].data << " "
              << e.Y.data[1].data << ") (" << e.Z.data[0].data << " " << e.Z.data[1].data << ")" << std::endl;
}

template<typename ValueType, std::size_t N>
typename std::enable_if<std::is_unsigned<ValueType>::value, std::vector<std::array<ValueType, N>>>::type
    generate_random_data(std::size_t leaf_number) {
    std::vector<std::array<ValueType, N>> v;
    for (std::size_t i = 0; i < leaf_number; ++i) {
        std::array<ValueType, N> leaf;
        std::generate(std::begin(leaf), std::end(leaf),
                      [&]() { return std::rand() % (std::numeric_limits<ValueType>::max() + 1); });
        v.emplace_back(leaf);
    }
    return v;
}

template<typename Endianness, typename Hash, std::size_t Arity>
void test_merkle_proof(std::size_t tree_depth) {

    using namespace nil::crypto3::marshalling;
    using merkle_tree_type = nil::crypto3::containers::merkle_tree<Hash, Arity>;
    using merkle_proof_type = nil::crypto3::containers::merkle_proof<Hash, Arity>;
    using merkle_proof_marshalling_type =
        types::merkle_proof<nil::marshalling::field_type<Endianness>, merkle_proof_type>;

    std::size_t leafs_number = 1 << tree_depth;
    auto data = generate_random_data<std::uint8_t, 32>(leafs_number);
    merkle_tree_type tree(data);
    std::size_t proof_idx = std::rand() % leafs_number;
    merkle_proof_type proof(tree, proof_idx);

    auto filled_merkle_proof = types::fill_merkle_proof<merkle_proof_type, Endianness>(proof);
    merkle_proof_type _proof = types::make_merkle_proof<merkle_proof_type, Endianness>(filled_merkle_proof);
    BOOST_CHECK(proof == _proof);

    std::vector<std::uint8_t> cv;
    cv.resize(filled_merkle_proof.length(), 0x00);
    auto write_iter = cv.begin();
    nil::marshalling::status_type status = filled_merkle_proof.write(write_iter, cv.size());

    merkle_proof_marshalling_type test_val_read;
    auto read_iter = cv.begin();
    status = test_val_read.read(read_iter, cv.size());
    merkle_proof_type constructed_val_read = types::make_merkle_proof<merkle_proof_type, Endianness>(test_val_read);
    BOOST_CHECK(proof == constructed_val_read);
}

BOOST_AUTO_TEST_SUITE(marshalling_merkle_proof_test_suite)

BOOST_AUTO_TEST_CASE(marshalling_merkle_proof_test) {
    test_merkle_proof<nil::marshalling::option::big_endian, nil::crypto3::hashes::sha2<256>, 2>(5);
}

BOOST_AUTO_TEST_SUITE_END()
