pragma solidity >=0.5.0;

// If there are no specs for events, we don't check them by default
// for compatibility with unannotated contracts
contract NoEmitsSpecs {

    event Increased();
    uint x;

    function increase() public {
        x++;
        emit Increased();
    }
}