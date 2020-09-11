// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract ECRecover {

  /// @notice postcondition ok
  function verifyTwice(bytes32 h, uint8 v, bytes32 r, bytes32 s) public pure returns (bool ok) {
    address a1 = ecrecover(h, v, r, s);
    address a2 = ecrecover(h, v, r, s);
    ok = (a1 == a2);
  }

  function() external payable {
    bytes32 h = keccak256('yas');
    uint8 v = 0x2a;
    bytes32 r = 0xc19db245478a06032e69cdbd2b54e648b78431d0a47bd1fbab18f79f820ba407;
    bytes32 s = 0x466e37adbe9e84541cab97ab7d290f4a64a5825c876d22109f3bf813254e8601;
    assert(verifyTwice(h, v, r, s));
  }
}
