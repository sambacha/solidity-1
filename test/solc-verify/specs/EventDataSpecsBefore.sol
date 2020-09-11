// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Token {
    mapping(address=>uint) balances;

    /// @notice tracks-changes-in balances
    /// @notice precondition balances[from] >= amount
    /// @notice postcondition balances[from] == __verifier_before_uint(balances[from]) - amount
    /// @notice postcondition balances[to] == __verifier_before_uint(balances[to]) + amount
    event transferred(address from, address to, uint amount);

    /// @notice emits transferred
    function transfer(address to, uint amount) public {
        require(balances[msg.sender] >= amount);
        require(msg.sender != to);
        balances[msg.sender] -= amount;
        balances[to] += amount;
        emit transferred(msg.sender, to, amount);
    }
}
