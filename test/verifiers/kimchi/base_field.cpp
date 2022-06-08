//---------------------------------------------------------------------------//
// Copyright (c) 2021-2022 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2021-2022 Nikita Kaskov <nbering@nil.foundation>
// Copyright (c) 2022 Ilia Shirobokov <i.shirobokov@nil.foundation>
// Copyright (c) 2022 Alisa Cherniaeva <a.cherniaeva@nil.foundation>
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

#define BOOST_TEST_MODULE blueprint_plonk_base_field_test

#include <boost/test/unit_test.hpp>

#include <nil/crypto3/algebra/curves/vesta.hpp>
#include <nil/crypto3/algebra/fields/arithmetic_params/vesta.hpp>
#include <nil/crypto3/algebra/random_element.hpp>

#include <nil/crypto3/hash/algorithm/hash.hpp>
#include <nil/crypto3/hash/sha2.hpp>
#include <nil/crypto3/hash/keccak.hpp>

#include <nil/crypto3/zk/snark/arithmetization/plonk/params.hpp>
//#include <nil/crypto3/zk/components/systems/snark/plonk/kimchi/detail/transcript_fr.hpp>

#include <nil/crypto3/zk/blueprint/plonk.hpp>
#include <nil/crypto3/zk/assignment/plonk.hpp>
#include <nil/crypto3/zk/components/algebra/curves/pasta/plonk/types.hpp>
#include <nil/crypto3/zk/components/systems/snark/plonk/kimchi/verifier_base_field.hpp>
#include <nil/crypto3/zk/components/systems/snark/plonk/kimchi/batch_verify_base_field.hpp>

#include "test_plonk_component.hpp"
using namespace nil::crypto3;

BOOST_AUTO_TEST_SUITE(blueprint_plonk_kimchi_base_field_test_suite)

BOOST_AUTO_TEST_CASE(blueprint_plonk_kimchi_base_field_test_suite) {

    using curve_type = algebra::curves::vesta;
    using BlueprintFieldType = typename curve_type::base_field_type;
    constexpr std::size_t WitnessColumns = 15;
    constexpr std::size_t PublicInputColumns = 1;
    constexpr std::size_t ConstantColumns = 1;
    constexpr std::size_t SelectorColumns = 10;
    using ArithmetizationParams = zk::snark::plonk_arithmetization_params<WitnessColumns,
        PublicInputColumns, ConstantColumns, SelectorColumns>;
    using ArithmetizationType = zk::snark::plonk_constraint_system<BlueprintFieldType,
                ArithmetizationParams>;
    using AssignmentType = zk::blueprint_assignment_table<ArithmetizationType>;
    using hash_type = nil::crypto3::hashes::keccak_1600<256>;
    using var_ec_point = typename zk::components::var_ec_point<BlueprintFieldType>;
    constexpr std::size_t Lambda = 40;
    constexpr static const std::size_t batch_size = 1;
    constexpr static const std::size_t lr_rounds = 1;
    constexpr static const std::size_t lagrange_bases_size = 1;
    constexpr static const std::size_t size = 8;
    constexpr static const std::size_t comm_size = 1;
    //constexpr static const std::size_t n_2 = ceil(log2(n));
    //constexpr static const std::size_t padding = (1 << n_2) - n;
    constexpr static const std::size_t shifted_commitment_type_size = 2;
    //constexpr static const std::size_t bases_size = n + padding + 1 + (1 + 1 + 2*lr_rounds + shifted_commitment_type_size + 1)* batch_size;
    
    constexpr static const std::size_t max_unshifted_size = 1;
    constexpr static const std::size_t proof_len = 1;

    constexpr static std::size_t public_input_size = 3;
    constexpr static std::size_t alpha_powers_n = 5;
    constexpr static std::size_t max_poly_size = 32;
    constexpr static std::size_t eval_rounds = 5;

    constexpr static std::size_t witness_columns = 15;
    constexpr static std::size_t perm_size = 7;
    constexpr static std::size_t lookup_table_size = 1;
    constexpr static bool use_lookup = false;

    constexpr static std::size_t srs_len = 1;

    using kimchi_params = zk::components::kimchi_params_type<witness_columns, perm_size,
        use_lookup, lookup_table_size,
        alpha_powers_n, public_input_size>;
    using commitment_params = zk::components::kimchi_commitment_params_type<eval_rounds, max_poly_size,
        srs_len>;

    using component_type = zk::components::base_field<ArithmetizationType, curve_type, 
        kimchi_params, commitment_params, batch_size, shifted_commitment_type_size,
        size, max_unshifted_size, proof_len,lagrange_bases_size,
                                                            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14>;

    using shifted_commitment_type = typename 
                        zk::components::kimchi_shifted_commitment_type<BlueprintFieldType, 
                            commitment_params::shifted_commitment_split>;

    using opening_proof_type = typename 
                        zk::components::kimchi_opening_proof<BlueprintFieldType, commitment_params::eval_rounds>;
    using var = zk::snark::plonk_variable<BlueprintFieldType>;

    using binding = typename zk::components::binding<ArithmetizationType,
                        BlueprintFieldType, commitment_params>;

    //zk::snark::pickles_proof<curve_type> kimchi_proof = test_proof();

    std::vector<typename BlueprintFieldType::value_type> public_input;
    std::vector<var_ec_point> shifted_var;
    std::vector<var_ec_point> unshifted_var;
    for(std::size_t i = 0; i < 14; i++) {
        curve_type::template g1_type<algebra::curves::coordinates::affine>::value_type shifted = 
        algebra::random_element<curve_type::template g1_type<algebra::curves::coordinates::affine>>();

        public_input.push_back(shifted.X);
        public_input.push_back(shifted.Y);

        shifted_var.push_back({var(0, i*4, false, var::column_type::public_input), var(0, i*4 + 1, false, var::column_type::public_input)});

        curve_type::template g1_type<algebra::curves::coordinates::affine>::value_type unshifted = 
        algebra::random_element<curve_type::template g1_type<algebra::curves::coordinates::affine>>();

        public_input.push_back(unshifted.X);
        public_input.push_back(unshifted.Y);

        unshifted_var.push_back({var(0, i*4 + 2, false, var::column_type::public_input), var(0, i*4 + 3, false, var::column_type::public_input)});
    }
    std::vector<shifted_commitment_type> witness_comm = {{{shifted_var[0]}, {unshifted_var[0]}}};
    std::vector<shifted_commitment_type> sigma_comm = {{{shifted_var[1]}, {unshifted_var[1]}}};
    std::vector<shifted_commitment_type> coefficient_comm = {{{shifted_var[2]}, {unshifted_var[2]}}};
    std::vector<shifted_commitment_type> oracles_poly_comm = {{{shifted_var[3]}, {unshifted_var[3]}}}; // to-do: get in the component from oracles
    shifted_commitment_type lookup_runtime_comm = {{shifted_var[4]}, {unshifted_var[4]}};
    shifted_commitment_type table_comm = {{shifted_var[5]}, {unshifted_var[5]}};
    std::vector<shifted_commitment_type> lookup_sorted_comm {{{shifted_var[6]}, {unshifted_var[6]}}};
    std::vector<shifted_commitment_type> lookup_selectors_comm = {{{shifted_var[7]}, {unshifted_var[7]}}};
    std::vector<shifted_commitment_type> selectors_comm = {{{shifted_var[8]}, {unshifted_var[8]}}};
    shifted_commitment_type lookup_agg_comm = {{shifted_var[9]}, {unshifted_var[9]}};
    shifted_commitment_type z_comm = {{shifted_var[10]}, {unshifted_var[10]}};
    shifted_commitment_type t_comm = {{shifted_var[11]}, {unshifted_var[11]}};
    shifted_commitment_type generic_comm = {{shifted_var[12]}, {unshifted_var[12]}};
    shifted_commitment_type psm_comm = {{shifted_var[13]}, {unshifted_var[13]}};

    curve_type::template g1_type<algebra::curves::coordinates::affine>::value_type L = 
    algebra::random_element<curve_type::template g1_type<algebra::curves::coordinates::affine>>();

    public_input.push_back(L.X);
    public_input.push_back(L.Y);

    var_ec_point L_var = {var(0, 56, false, var::column_type::public_input), var(0, 57, false, var::column_type::public_input)};

    curve_type::template g1_type<algebra::curves::coordinates::affine>::value_type R = 
    algebra::random_element<curve_type::template g1_type<algebra::curves::coordinates::affine>>();

    public_input.push_back(R.X);
    public_input.push_back(R.Y);

    var_ec_point R_var = {var(0, 58, false, var::column_type::public_input), var(0, 59, false, var::column_type::public_input)};

    curve_type::template g1_type<algebra::curves::coordinates::affine>::value_type delta = 
    algebra::random_element<curve_type::template g1_type<algebra::curves::coordinates::affine>>();

    public_input.push_back(delta.X);
    public_input.push_back(delta.Y);

    var_ec_point delta_var = {var(0, 60, false, var::column_type::public_input), var(0, 61, false, var::column_type::public_input)};

    curve_type::template g1_type<algebra::curves::coordinates::affine>::value_type G = 
    algebra::random_element<curve_type::template g1_type<algebra::curves::coordinates::affine>>();

    public_input.push_back(G.X);
    public_input.push_back(G.Y);

    var_ec_point G_var = {var(0, 62, false, var::column_type::public_input), var(0, 63, false, var::column_type::public_input)};

    opening_proof_type o_var = {{L_var}, {R_var}, delta_var, G_var};

    std::array<curve_type::base_field_type::value_type, size> scalars;

    std::vector<var> scalars_var(size);

    for (std::size_t i = 0; i < size; i++) {
        scalars[i] = algebra::random_element<curve_type::base_field_type>();
        public_input.push_back(scalars[i]);
        scalars_var[i] = var(0, 74 + i, false, var::column_type::public_input);
    }

     curve_type::template g1_type<algebra::curves::coordinates::affine>::value_type lagrange_bases = 
    algebra::random_element<curve_type::template g1_type<algebra::curves::coordinates::affine>>();

    public_input.push_back(lagrange_bases.X);
    public_input.push_back(lagrange_bases.Y);

    var_ec_point lagrange_bases_var = {var(0, 65, false, var::column_type::public_input), var(0, 66, false, var::column_type::public_input)};

    typename curve_type::base_field_type::value_type Pub = algebra::random_element<curve_type::base_field_type>();
    public_input.push_back(Pub);
    var Pub_var = var(0, 67, false, var::column_type::public_input);

    typename curve_type::base_field_type::value_type zeta_to_srs_len = algebra::random_element<curve_type::base_field_type>();
    public_input.push_back(zeta_to_srs_len);
    var zeta_to_srs_len_var = var(0, 68, false, var::column_type::public_input);

    typename curve_type::base_field_type::value_type zeta_to_domain_size_minus_1 = algebra::random_element<curve_type::base_field_type>();
    public_input.push_back(zeta_to_domain_size_minus_1);
    var zeta_to_domain_size_minus_1_var = var(0, 69, false, var::column_type::public_input);


    curve_type::template g1_type<algebra::curves::coordinates::affine>::value_type H = 
    algebra::random_element<curve_type::template g1_type<algebra::curves::coordinates::affine>>();

    public_input.push_back(H.X);
    public_input.push_back(H.Y);

    var_ec_point H_var = {var(0, 70, false, var::column_type::public_input), var(0, 71, false, var::column_type::public_input)};

    curve_type::template g1_type<algebra::curves::coordinates::affine>::value_type PI_G = 
    algebra::random_element<curve_type::template g1_type<algebra::curves::coordinates::affine>>();

    public_input.push_back(PI_G.X);
    public_input.push_back(PI_G.Y);

    var_ec_point PI_G_var = {var(0, 72, false, var::column_type::public_input), var(0, 73, false, var::column_type::public_input)};

    constexpr static const std::size_t bases_size = srs_len + 1 + (1 + 1 + 2*lr_rounds + shifted_commitment_type_size + 1)* batch_size;
    std::array<curve_type::base_field_type::value_type, bases_size> batch_scalars;

    std::vector<var> batch_scalars_var(bases_size);

    for (std::size_t i = 0; i < bases_size; i++) {
        batch_scalars[i] = algebra::random_element<curve_type::base_field_type>();
        public_input.push_back(batch_scalars[i]);
        batch_scalars_var[i] = var(0, 74 + i, false, var::column_type::public_input);
    }
    curve_type::base_field_type::value_type cip = algebra::random_element<curve_type::base_field_type>();

    public_input.push_back(cip);

    var cip_var = var(0, 74 + bases_size, false, var::column_type::public_input);   

    typename component_type::params_type::commitments commitments = {{witness_comm}, {sigma_comm},
                             {coefficient_comm},
                             {oracles_poly_comm}, // to-do: get in the component from oracles
                            lookup_runtime_comm,
                             table_comm,
                             {lookup_sorted_comm},
                             {lookup_selectors_comm},
                             {selectors_comm}, 
                             lookup_agg_comm,
                             z_comm,
                             t_comm,
                             generic_comm,
                             psm_comm};
    /*zk::components::kimchi_transcript<ArithmetizationType, curve_type, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                        11, 12, 13, 14> transcript;*/
    typename component_type::params_type::var_proof proof_var = {/*transcript, */ commitments, o_var, {scalars_var}}; 
    typename component_type::params_type::public_input PI_var = {{lagrange_bases_var},
                            {Pub_var},
                            zeta_to_srs_len_var,
                            zeta_to_domain_size_minus_1_var};
    typename component_type::params_type::result input = {{proof_var}, {H_var, {PI_G_var}}, PI_var};

    typename binding::fr_data<var, batch_size> fr_data = {batch_scalars_var, {cip_var}};
    typename binding::fq_data<var> fq_data;

    typename component_type::params_type params = {fr_data, fq_data, input};
 
    auto result_check = [](AssignmentType &assignment, 
        component_type::result_type &real_res) {
    };

    test_component<component_type, BlueprintFieldType, ArithmetizationParams, hash_type, Lambda> (params, public_input, result_check); 
}

BOOST_AUTO_TEST_SUITE_END()