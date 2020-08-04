//---------------------------------------------------------------------------//
// Copyright (c) 2020 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2020 Nikita Kaskov <nbering@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#ifndef ALGEBRA_FF_BN128_INIT_HPP
#define ALGEBRA_FF_BN128_INIT_HPP

#include <nil/algebra/pairing/include/bn.h>

#include <nil/algebra/curves/detail/bn128/bn128_g1.hpp>
#include <nil/algebra/curves/detail/bn128/bn128_g2.hpp>
#include <nil/algebra/curves/detail/bn128/bn128_gt.hpp>

#include <nil/algebra/fields/fp.hpp>

#include <boost/multiprecision/modular/base_params.hpp>

namespace nil {
    namespace algebra {

        template <typename ModulusBits, typename GeneratorBits>
        using params_type = arithmetic_params<fp<ModulusBits, GeneratorBits>>;

        template <typename ModulusBits, typename GeneratorBits>
        using modulus_type = params_type<ModulusBits, GeneratorBits>::modulus_type;

        template <typename ModulusBits, typename GeneratorBits>
        using fp_type = fp<ModulusBits, GeneratorBits>;

        template <typename ModulusBits, typename GeneratorBits>
        using fp_value_type = element<fp_type<ModulusBits, GeneratorBits>>;

        template <typename ModulusBits, typename GeneratorBits>
        using fp2_type = fp2<ModulusBits, GeneratorBits>;

        template <typename ModulusBits, typename GeneratorBits>
        using fp2_value_type = element<fp2_type<ModulusBits, GeneratorBits>>;

        template <typename ModulusBits, typename GeneratorBits>
        using fp12_type = fp12_2over3over2<ModulusBits, GeneratorBits>;

        template <typename ModulusBits, typename GeneratorBits>
        using fp12_value_type = element<fp12_type<ModulusBits, GeneratorBits>>;

        const mp_size_t bn128_r_bitcount = 254;
        const mp_size_t bn128_q_bitcount = 254;

        NumberType bn128_modulus_r;
        NumberType bn128_modulus_q;

        fp_value_type bn128_coeff_b;
        size_t bn128_Fq_s;
        fp_value_type bn128_Fq_nqr_to_t;
        mie::Vuint bn128_Fq_t_minus_1_over_2;

        fp2_value_type bn128_twist_coeff_b;
        size_t bn128_Fq2_s;
        fp2_value_type bn128_Fq2_nqr_to_t;
        mie::Vuint bn128_Fq2_t_minus_1_over_2;

        template<typename NumberType>
        void init_bn128_params() {

            /* additional parameters for square roots in Fq/Fq2 */
            bn128_coeff_b = fp_value_type(3);
            bn128_Fq_s = 1;
            bn128_Fq_nqr_to_t = fp_value_type("21888242871839275222246405745257275088696311157297823662689037894645226208582");
            bn128_Fq_t_minus_1_over_2 =
                mie::Vuint("5472060717959818805561601436314318772174077789324455915672259473661306552145");

            bn128_twist_coeff_b =
                fp2_value_type({fp_value_type("19485874751759354771024239261021720505790618469301721065564631296452457478373"),
                        fp_value_type("266929791119991161246907387137283842545076965332900288569378510910307636690")});
            bn128_Fq2_s = 4;
            bn128_Fq2_nqr_to_t =
                fp2_value_type({fp_value_type("5033503716262624267312492558379982687175200734934877598599011485707452665730"),
                        fp_value_type("314498342015008975724433667930697407966947188435857772134235984660852259084")});
            bn128_Fq2_t_minus_1_over_2 = mie::Vuint(
                "14971724250519463826312126413021210649976634891596900701138993820439690427699319920245032869357433"
                "49909963"
                "2259837909383182382988566862092145199781964621");

            /* choice of group G1 */
            bn128_G1::G1_zero.coord[0] = fp_value_type(1);
            bn128_G1::G1_zero.coord[1] = fp_value_type(1);
            bn128_G1::G1_zero.coord[2] = fp_value_type(0);

            bn128_G1::G1_one.coord[0] = fp_value_type(1);
            bn128_G1::G1_one.coord[1] = fp_value_type(2);
            bn128_G1::G1_one.coord[2] = fp_value_type(1);

            /* choice of group G2 */
            bn128_G2::G2_zero.coord[0] = fp2_value_type({fp_value_type(1), fp_value_type(0)});
            bn128_G2::G2_zero.coord[1] = fp2_value_type({fp_value_type(1), fp_value_type(0)});
            bn128_G2::G2_zero.coord[2] = fp2_value_type({fp_value_type(0), fp_value_type(0)});

            bn128_G2::G2_one.coord[0] =
                fp2_value_type({fp_value_type("15267802884793550383558706039165621050290089775961208824303765753922461897946"),
                        fp_value_type("9034493566019742339402378670461897774509967669562610788113215988055021632533")});
            bn128_G2::G2_one.coord[1] =
                fp2_value_type({fp_value_type("644888581738283025171396578091639672120333224302184904896215738366765861164"),
                        fp_value_type("20532875081203448695448744255224543661959516361327385779878476709582931298750")});
            bn128_G2::G2_one.coord[2] = fp2_value_type({fp_value_type(1), fp_value_type(0)});

            bn128_GT::GT_one.elem = fp12_value_type(1);
        }

        class bn128_G1;
        class bn128_G2;
        class bn128_GT;
        typedef bn128_GT bn128_Fq12;

    }    // namespace algebra
}    // namespace nil
#endif    // ALGEBRA_FF_BN128_INIT_HPP
