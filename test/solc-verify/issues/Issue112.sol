pragma solidity ^0.5.0;

contract test2 {
    mapping(address => uint) public t;
}

contract test  {

    test2 t2;
    address a;

    /// @notice precondition t2.t(a) == 0
    function f() public view {
        assert(t2.t(a) == 0);
    }
}