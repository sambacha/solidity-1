// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract PayableFunctions {

    /**
     * @notice postcondition r == param + msg.value
     */
    function rcv(uint param) public payable returns (uint r) {
        return param + msg.value;
    }
}

contract Payable {

    PayableFunctions p;

    receive() external payable {
        assert(transfer(1) == 2);
    }

    constructor() {
        p = new PayableFunctions();
    }

    function transfer(uint amount) private returns (uint) {
        require(address(this).balance >= amount);
        return p.rcv{value: amount}(1);
    }

    function transferNested(uint amount) public returns (uint) {
        require(amount >= 0);
        require(address(this).balance >= amount + 3);
        // Calling a payable function multiple times with checking for balance
        // only in the beginning will fail, because the called function might
        // modify our balance as well. However, this is an expected failure and
        // currently we are not reporting it.
        return p.rcv{value: p.rcv{value: 1}(2)}(p.rcv{value: amount}(3));
    }
}
