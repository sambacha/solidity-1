// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract BitPreciseExt {
    function u32tou40(uint32 x) private pure returns (uint40) { return x; }
    function s16tos48(int16 x) private pure returns (int48) { return x; }
    function u32tos40(uint32 x) private pure returns (int40)  { return x; }

    receive() external payable {
        assert(u32tou40(123) == 123);
        assert(s16tos48(123) == 123);
        assert(s16tos48(-123) == -123);
        assert(u32tos40(123) == 123);
    }
}
