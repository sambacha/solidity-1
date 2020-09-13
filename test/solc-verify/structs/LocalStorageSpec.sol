// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract LSS_A {
  struct S {
      int x;
  }
  function setA0(S storage s_ptr) internal {
    s_ptr.x = 0;
  }
}

contract LSS_B is LSS_A {
  S s;
  /// @notice postcondition s.x == 0
  function setB0(S storage s_ptr) internal {
    setA0(s_ptr);
  }
}

contract LocalStorageSpec is LSS_B {
  S s1;
  function setC0(S storage s_ptr) internal {
    setB0(s_ptr);
  }
  receive() external payable {
    setC0(s1);
    assert(s1.x == 0);
  }

}
