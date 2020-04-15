pragma solidity >=0.5.0;

contract EventDataSpecsOnlyTrack {
    int x;

    /// @notice tracks-changes-in x
    event changed();

    /// @notice emits changed
    function f_ok() public {
        x++;
        emit changed();
    }

    /// @notice emits changed
    function f_incorrect1() public {
        x++;
        emit changed();
        x++;
    }

    // TODO: currently passes, but should fail
    //function f_incorrect2() public {
    //    x++;
    //}
}