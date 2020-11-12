//---------------------------------------------------------------------------//
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2020 Nikita Kaskov <nbering@nil.foundation>
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

#define BOOST_TEST_MODULE sha2_256_component_test

#include <boost/test/unit_test.hpp>

#include <nil/crypto3/algebra/curves/mnt4.hpp>
#include <nil/crypto3/algebra/fields/mnt4/base_field.hpp>
#include <nil/crypto3/algebra/fields/mnt4/scalar_field.hpp>
#include <nil/crypto3/algebra/fields/arithmetic_params/mnt4.hpp>
#include <nil/crypto3/algebra/curves/params/multiexp/mnt4.hpp>
#include <nil/crypto3/algebra/curves/params/wnaf/mnt4.hpp>

#include <nil/crypto3/zk/snark/components/hashes/sha256/sha256_component.hpp>

using namespace nil::crypto3::algebra;
using namespace nil::crypto3;
using namespace nil::crypto3::zk::snark;

template<typename FieldType>
void test_two_to_one() {
    blueprint<FieldType> pb;

    digest_variable<FieldType> left(pb, hashes::sha2<256>::digest_bits);
    digest_variable<FieldType> right(pb, hashes::sha2<256>::digest_bits);
    digest_variable<FieldType> output(pb, hashes::sha2<256>::digest_bits);

    sha256_two_to_one_hash_component<FieldType> f(pb, left, right, output);
    f.generate_r1cs_constraints();
    printf("Number of constraints for sha256_two_to_one_hash_component: %zu\n", pb.num_constraints());

    const std::vector<bool> left_bv, right_bv, hash_bv;
    detail::pack_to<stream_endian::little_octet_big_bit, 32, 1, std::array<std::uint32_t, 8>>(
        {0x426bc2d8, 0x4dc86782, 0x81e8957a, 0x409ec148, 
         0xe6cffbe8, 0xafe6ba4f, 0x9c6f1978, 0xdd7af7e9},
        left_bv.begin());
    detail::pack_to<stream_endian::little_octet_big_bit, 32, 1, std::array<std::uint32_t, 8>>(
        {0x038cce42, 0xabd366b8, 0x3ede7e00, 0x9130de53, 
         0x72cdf73d, 0xee825114, 0x8cb48d1b, 0x9af68ad0},
        right_bv.begin());

    detail::pack_to<stream_endian::little_octet_big_bit, 32, 1, std::array<std::uint32_t, 8>>(
        {0xeffd0b7f, 0x1ccba116, 0x2ee816f7, 0x31c62b48, 
         0x59305141, 0x990e5c0a, 0xce40d33d, 0x0b1167d1},
        hash_bv.begin());

    left.generate_r1cs_witness(left_bv);
    right.generate_r1cs_witness(right_bv);

    f.generate_r1cs_witness();
    output.generate_r1cs_witness(hash_bv);

    BOOST_CHECK(pb.is_satisfied());
}

BOOST_AUTO_TEST_SUITE(sha2_256_component_test_suite)

BOOST_AUTO_TEST_CASE(sha2_256_component_test_case) {
    test_two_to_one<typename curves::mnt4<298>::scalar_field_type>();
}

BOOST_AUTO_TEST_SUITE_END()
