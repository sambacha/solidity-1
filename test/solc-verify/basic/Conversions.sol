// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract SomeContract {
    uint public stateVar;

    function someFunc() public view returns (uint) {
        return stateVar;
    }
}

contract OtherContract {
    function otherFunc(address addr) public view returns (uint) {
        return SomeContract(addr).someFunc();
    }
}

contract Conversions {
    function someFunc(uint x) public pure returns (int) {
        int y = int(x);
        return y;
    }
}
