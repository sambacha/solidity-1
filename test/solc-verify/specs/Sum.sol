// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

/**
 * @notice invariant __verifier_sum_uint(user_balances) == total_supply
 */
contract Sum {
    uint8 total_supply;
    mapping(address=>uint8) user_balances;

    function deposit(uint8 amount) public {
        total_supply += amount;
        user_balances[msg.sender] += amount;
    }

    function withdraw() public {
        total_supply -= user_balances[msg.sender];
        user_balances[msg.sender] = 0;
    }

    function withdraw_incorrect() public {
        total_supply -= user_balances[msg.sender];
        user_balances[msg.sender] = 12;
    }
}
