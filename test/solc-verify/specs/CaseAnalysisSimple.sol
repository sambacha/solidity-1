pragma solidity >=0.5.0;

contract CaseAnalysisSimple {
    int x;
    int y;

    /// @notice specification
    /// [
    ///     case x > 0: y == 1;
    ///     case x < 0: y == -1;
    /// ]
    function correct() public {
        if (x > 0) y = 1;
        else if (x < 0) y = -1;
    }

    /// @notice specification
    /// [
    ///     case x > 0: y == 1;
    ///     case x < 0: y == -1;
    /// ]
    function wrongdefault() public {
        if (x > 0) y = 1;
        y = -1;
    }

}