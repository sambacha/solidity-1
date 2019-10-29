pragma solidity >=0.5.0;

contract A {
    mapping(address=>int) public m1;
    mapping(address=>mapping(int=>int)) public m2;

    /// @notice postcondition this.m1(key) == value
    function setm1(address key, int value) public {
        m1[key] = value;
    }

    /// @notice postcondition this.m2(key1, key2) == value
    function setm2(address key1, int key2, int value) public {
        m2[key1][key2] = value;
    }
}