//---------------------------------------------------------------------------//
// Copyright (c) 2024 Alexey Yashunsky <a.yashunsky@nil.foundation>
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

#define BOOST_TEST_MODULE zkevm_err0_test

#include <boost/test/unit_test.hpp>

//#include <nil/crypto3/algebra/fields/arithmetic_params/goldilocks64.hpp>
#include "nil/crypto3/algebra/fields/pallas/base_field.hpp"

#include <nil/blueprint/blueprint/plonk/circuit.hpp>
#include <nil/blueprint/blueprint/plonk/assignment.hpp>

#include <nil/blueprint/zkevm/zkevm_circuit.hpp>
#include "../opcode_tester.hpp"

using namespace nil::blueprint;
using namespace nil::crypto3::algebra;

BOOST_AUTO_TEST_SUITE(zkevm_err0_test_suite)

BOOST_AUTO_TEST_CASE(zkevm_err0_test_1) {
    using field_type = fields::pallas_base_field;
    using arithmentization_type = nil::crypto3::zk::snark::plonk_constraint_system<field_type>;
    using assignment_type = assignment<arithmentization_type>;
    using circuit_type = circuit<arithmentization_type>;
    using zkevm_machine_type = zkevm_machine_interface;
    assignment_type assignment(0, 0, 0, 0);
    circuit_type circuit;
    zkevm_circuit<field_type> evm_circuit(assignment, circuit, 15, 500);
    nil::crypto3::zk::snark::pack_lookup_tables_horizontal(
        circuit.get_reserved_indices(),
        circuit.get_reserved_tables(),
        circuit.get_reserved_dynamic_tables(),
        circuit, assignment,
        assignment.rows_amount(),
        65536
    );

    zkevm_table<field_type> zkevm_table(evm_circuit, assignment);
    zkevm_opcode_tester opcode_tester;

    // incorrect test logic, but we have no memory operations so
    opcode_tester.push_opcode(zkevm_opcode::ADD);
    opcode_tester.push_opcode(zkevm_opcode::RETURN);

    zkevm_machine_type machine = get_empty_machine(opcode_tester.get_bytecode(), zkevm_keccak_hash(opcode_tester.get_bytecode()));
    while(true) {
        machine.apply_opcode(opcode_tester.get_opcode_by_pc(machine.pc_next()).first, opcode_tester.get_opcode_by_pc(machine.pc_next()).second);
        zkevm_table.assign_opcode(machine);
        if( machine.tx_finish()) break;
    }

    typename zkevm_circuit<field_type>::bytecode_table_component::input_type bytecode_input;
    bytecode_input.new_bytecode(opcode_tester.get_bytecode());
    bytecode_input.new_bytecode({0x60,0x40,0x60,0x80, 0xF3});

    zkevm_table.finalize_test(bytecode_input);
    // assignment.export_table(std::cout);
    // circuit.export_circuit(std::cout);
    nil::crypto3::zk::snark::basic_padding(assignment);
    BOOST_ASSERT(is_satisfied(circuit, assignment) == true);
}

BOOST_AUTO_TEST_CASE(zkevm_err0_test_2) {
    using field_type = fields::pallas_base_field;
    using arithmentization_type = nil::crypto3::zk::snark::plonk_constraint_system<field_type>;
    using assignment_type = assignment<arithmentization_type>;
    using circuit_type = circuit<arithmentization_type>;
    using zkevm_machine_type = zkevm_machine_interface;
    assignment_type assignment(0, 0, 0, 0);
    circuit_type circuit;
    zkevm_circuit<field_type> evm_circuit(assignment, circuit, 2060, 65536);
    nil::crypto3::zk::snark::pack_lookup_tables_horizontal(
        circuit.get_reserved_indices(),
        circuit.get_reserved_tables(),
        circuit.get_reserved_dynamic_tables(),
        circuit, assignment,
        assignment.rows_amount(),
        65536
    );

    zkevm_table<field_type> zkevm_table(evm_circuit, assignment);
    zkevm_opcode_tester opcode_tester;

    // incorrect test logic, but we have no memory operations so
    for( std::size_t i = 0; i < 1025; i++ ){
        opcode_tester.push_opcode(zkevm_opcode::PUSH1, 0);
    }
    opcode_tester.push_opcode(zkevm_opcode::RETURN);

    zkevm_machine_type machine = get_empty_machine(opcode_tester.get_bytecode(), zkevm_keccak_hash(opcode_tester.get_bytecode()));
    while(true) {
        machine.apply_opcode(opcode_tester.get_opcode_by_pc(machine.pc_next()).first, opcode_tester.get_opcode_by_pc(machine.pc_next()).second);
        zkevm_table.assign_opcode(machine);
        if( machine.tx_finish()) break;
    }

    typename zkevm_circuit<field_type>::bytecode_table_component::input_type bytecode_input;
    bytecode_input.new_bytecode(opcode_tester.get_bytecode());
    bytecode_input.new_bytecode({0x60,0x40,0x60,0x80, 0xF3});

    zkevm_table.finalize_test(bytecode_input);
    // assignment.export_table(std::cout);
    // circuit.export_circuit(std::cout);
    nil::crypto3::zk::snark::basic_padding(assignment);
    BOOST_ASSERT(is_satisfied(circuit, assignment) == true);
}

/*
BOOST_AUTO_TEST_CASE(zkevm_err0_test_2) {
    using field_type = fields::pallas_base_field;
    using arithmentization_type = nil::crypto3::zk::snark::plonk_constraint_system<field_type>;
    using assignment_type = assignment<arithmentization_type>;
    using circuit_type = circuit<arithmentization_type>;
    using zkevm_machine_type = zkevm_machine_interface;
    assignment_type assignment(0, 0, 0, 0);
    circuit_type circuit;
    zkevm_circuit<field_type> zkevm_circuit(assignment, circuit, 15);
    zkevm_table<field_type> zkevm_table(zkevm_circuit, assignment);
    zkevm_machine_type machine = get_empty_machine(8);

    machine.apply_opcode(zkevm_opcode::PUSH32, 0x1b70726fb8d3a24da9ff9647225a18412b8f010425938504d73ebc8801e2e016_big_uint257); zkevm_table.assign_opcode(machine);
    machine.apply_opcode(zkevm_opcode::PUSH32, 0x1b70726fb8d3a24da9ff9647225a18412b8f010425938504d73ebc8801e2e016_big_uint257); zkevm_table.assign_opcode(machine);
    // produce an out-of-gas error
    zkevm_table.finalize_test();

//    std::ofstream myfile;
//    myfile.open("test_assignment_err2.txt");
//    assignment.export_table(myfile);
//    myfile.close();

    // assignment.export_table(std::cout);
    // circuit.export_circuit(std::cout);
    nil::crypto3::zk::snark::basic_padding(assignment);
    BOOST_ASSERT(is_satisfied(circuit, assignment) == true);
}
*/
BOOST_AUTO_TEST_SUITE_END()
