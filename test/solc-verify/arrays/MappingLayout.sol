// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract MappingLayout {


    struct T {
        int z;
    }

    mapping(address=>T) ts;

    receive() external payable {
        require(msg.sender != address(this));
        ts[msg.sender] = T(1);
        ts[address(this)] = ts[msg.sender];

        assert(ts[address(this)].z == 1);
        ts[msg.sender].z = 2;
        assert(ts[address(this)].z == 1);
    }
}
