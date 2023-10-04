#ifndef __MODULAR_EXTERNAL_GATE_ARGUMENT_TEMPLATE_HPP__
#define __MODULAR_EXTERNAL_GATE_ARGUMENT_TEMPLATE_HPP__

#include <string>

namespace nil {
    namespace blueprint {
        std::string gate_evaluation_template = R"(
    function evaluate_gate_$GATE_ID$_be(
        bytes calldata blob,
        uint256 theta,
        uint256 theta_acc
    ) external pure returns (uint256 F, uint256) {
        uint256 sum;
        uint256 gate;
        uint256 prod;

$GATE_ASSEMBLY_CODE$

        return( F, theta_acc );
    }
)";
        std::string modular_external_gate_library_template = R"(
// SPDX-License-Identifier: Apache-2.0.
//---------------------------------------------------------------------------//
// Copyright (c)  2023 -- Generated by zkllvm-transpiler
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//---------------------------------------------------------------------------//
pragma solidity >=0.8.4;

import "../../../contracts/basic_marshalling.sol";

library gate_$TEST_NAME$_$GATE_LIB_ID${
    uint256 constant modulus = $MODULUS$;

    $GATES_COMPUTATION_CODE$
}
        )";
    }
}

#endif //__EXTERNAL_GATE_ARGUMENT_TEMPLATE_HPP__
