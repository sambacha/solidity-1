pragma solidity >=0.5.0;

contract Base {
    event ev(int x);
}

contract Derived is Base {
    event ev(int x, int y);

    /// @notice emits ev
    function f() public {
        emit ev(1, 2);
    }

    /// @notice emits ev
    function g() public {
        emit ev(1);
    }

}