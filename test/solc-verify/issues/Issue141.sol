pragma solidity >=0.5.0;

contract C {
    mapping(int=>int) data;

    /// @notice tracks-changes-in data
    event changed();

    function f() public {
        data[0] = 1;
    }

    int[] a;

    /// @notice tracks-changes-in a
    event add_element();

    /// @notice tracks-changes-in a
    event remove_element();

    function push(int x) public {
        a.push(x);
    }

    function pop() public {
        require(a.length > 0);
        a.pop();
    }

    function modify_tuple() public {
        require(a.length > 0);
        (a[0], data[0]) = (0, 0);
    }
}
