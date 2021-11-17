//---------------------------------------------------------------------------//
// Copyright (c) 2021 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2021 Nikita Kaskov <nbering@nil.foundation>
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

#define BOOST_TEST_MODULE blueprint_fixed_base_mul_zcash_component_test

#include <boost/test/unit_test.hpp>

#include <nil/crypto3/algebra/curves/babyjubjub.hpp>
#include <nil/crypto3/algebra/curves/jubjub.hpp>
#include <nil/crypto3/algebra/random_element.hpp>

#include <nil/crypto3/zk/components/algebra/curves/montgomery/element_g1.hpp>
#include <nil/crypto3/zk/components/algebra/curves/twisted_edwards/element_g1.hpp>
#include <nil/crypto3/zk/components/hashes/pedersen.hpp>

using namespace nil::crypto3;
using namespace nil::crypto3::zk;
using namespace nil::crypto3::algebra;

template<typename FieldParams>
void print_field_element(std::ostream &os, const typename fields::detail::element_fp<FieldParams> &e) {
    std::cout << e.data << std::endl;
}

template<typename Curve,
         typename HashToPointComponent = components::pedersen_to_point<Curve>,
         typename HashComponent = components::pedersen<Curve>>
void test_pedersen_default_params_component(
    const std::vector<bool> &in_bits,
    const typename HashToPointComponent::element_component::group_value_type &expected,
    const std::vector<bool> &expected_bits) {
    using field_type = typename HashToPointComponent::element_component::group_value_type::field_type;

    /// hashing to point
    components::blueprint<field_type> bp, bp_manual;
    components::blueprint_variable_vector<field_type> scalar, scalar_manual;
    scalar.allocate(bp, in_bits.size());
    scalar.fill_with_bits(bp, in_bits);
    scalar_manual.allocate(bp_manual, in_bits.size());
    scalar_manual.fill_with_bits(bp_manual, in_bits);

    // Auto allocation of the result
    HashToPointComponent hash_comp(bp, scalar);
    hash_comp.generate_r1cs_witness();
    hash_comp.generate_r1cs_constraints();
    BOOST_CHECK(expected.X == bp.lc_val(hash_comp.result.X));
    BOOST_CHECK(expected.Y == bp.lc_val(hash_comp.result.Y));
    BOOST_CHECK(bp.is_satisfied());

    // Manual allocation of the result
    typename HashToPointComponent::result_type result_manual(bp_manual);
    HashToPointComponent hash_comp_manual(bp_manual, scalar_manual, result_manual);
    hash_comp_manual.generate_r1cs_witness();
    hash_comp_manual.generate_r1cs_constraints();
    BOOST_CHECK(expected.X == bp_manual.lc_val(result_manual.X));
    BOOST_CHECK(expected.Y == bp_manual.lc_val(result_manual.Y));
    BOOST_CHECK(bp_manual.is_satisfied());

    /// hashing to bits
    components::blueprint<field_type> bp_bits, bp_bits_manual;
    components::blueprint_variable_vector<field_type> scalar_bits, scalar_bits_manual;
    scalar_bits.allocate(bp_bits, in_bits.size());
    scalar_bits.fill_with_bits(bp_bits, in_bits);
    scalar_bits_manual.allocate(bp_bits_manual, in_bits.size());
    scalar_bits_manual.fill_with_bits(bp_bits_manual, in_bits);

    // Auto allocation of the result
    HashComponent hash_comp_bits(bp_bits, scalar_bits);
    hash_comp_bits.generate_r1cs_witness();
    hash_comp_bits.generate_r1cs_constraints();
    BOOST_CHECK(expected_bits == hash_comp_bits.result.get_bits(bp_bits));
    BOOST_CHECK(bp_bits.is_satisfied());

    // Manual allocation of the result
    typename HashComponent::result_type result_bits_manual;
    result_bits_manual.allocate(bp_bits_manual, HashComponent::field_type::value_bits);
    HashComponent hash_comp_bits_manual(bp_bits_manual, scalar_bits_manual, result_bits_manual);
    hash_comp_bits_manual.generate_r1cs_witness();
    hash_comp_bits_manual.generate_r1cs_constraints();
    BOOST_CHECK(expected_bits == result_bits_manual.get_bits(bp_bits_manual));
    BOOST_CHECK(bp_bits_manual.is_satisfied());
}

// TODO: extend tests, add checks of wrong values
BOOST_AUTO_TEST_SUITE(blueprint_pedersen_manual_test_suite)

// test data generated by https://github.com/zcash-hackworks/zcash-test-vectors
BOOST_AUTO_TEST_CASE(pedersen_jubjub_sha256_default_params_test) {
    using curve_type = curves::jubjub;
    using field_type = typename curve_type::base_field_type;
    using field_value_type = typename field_type::value_type;
    using integral_type = typename field_type::integral_type;

    std::vector<bool> bits_to_hash = {0, 0, 0, 1, 1, 1};
    auto expected =
        typename curve_type::template g1_type<curves::coordinates::affine, curves::forms::twisted_edwards>::value_type(
            field_value_type(
                integral_type("3669431847238482802904025485408296241776002230868041345055738963615665974946")),
            field_value_type(
                integral_type("27924821127213629235056488929093463445821551452792195607066067950495472725010")));
    std::vector<bool> expected_bits = {
        0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1,
        0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1,
        0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
        1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0,
        1, 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0,
        1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0};
    test_pedersen_default_params_component<curve_type>(bits_to_hash, expected, expected_bits);

    bits_to_hash = std::vector<bool> {0, 0, 1};
    expected =
        typename curve_type::template g1_type<curves::coordinates::affine, curves::forms::twisted_edwards>::value_type(
            field_value_type(
                integral_type("37613883148175089126541491300600635192159391899451195953263717773938227311808")),
            field_value_type(
                integral_type("52287259411977570791304693313354699485314647509298698724706688571292689216990")));
    expected_bits = {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1,
                     1, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0,
                     1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1,
                     0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0,
                     1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1,
                     1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
                     1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0,
                     1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1};
    test_pedersen_default_params_component<curve_type>(bits_to_hash, expected, expected_bits);

    bits_to_hash = std::vector<bool> {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1,
                                      0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1,
                                      0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1,
                                      0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1,
                                      0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1,
                                      0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1,
                                      0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1};
    expected =
        typename curve_type::template g1_type<curves::coordinates::affine, curves::forms::twisted_edwards>::value_type(
            field_value_type(
                integral_type("42176130776060636907007595971304534904965322197894055434176666599102076910022")),
            field_value_type(
                integral_type("41298132615767455442973386625334423316246314118050839847545855695501416927077")));
    expected_bits = {0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1,
                     1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1,
                     0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0,
                     0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0,
                     1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1,
                     0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1,
                     1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1,
                     0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 1};
    test_pedersen_default_params_component<curve_type>(bits_to_hash, expected, expected_bits);

    bits_to_hash.resize(3 * 63 * 20);
    for (auto i = 0; i < bits_to_hash.size(); i++) {
        bits_to_hash[i] = std::vector<bool> {0, 0, 1}[i % 3];
    }
    expected =
        typename curve_type::template g1_type<curves::coordinates::affine, curves::forms::twisted_edwards>::value_type(
            field_value_type(
                integral_type("16831926627213193043296678235139527332739870606672735560230973395062624230202")),
            field_value_type(
                integral_type("29758113761493087483326459667018939508613372210858382541334106957041082715241")));
    expected_bits = {0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0,
                     0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1,
                     0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 1, 0,
                     0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1,
                     0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1,
                     1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1,
                     1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 0, 1,
                     1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0};
    test_pedersen_default_params_component<curve_type>(bits_to_hash, expected, expected_bits);
}

BOOST_AUTO_TEST_SUITE_END()