// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

/// @notice invariant forall (uint i) !(0 <= i && i < a.length) || (a[i] > 0)
contract C_positive {
  int[] a;

  function add0() public {
    if (a.length == 0) {
      a.push(0);
    } else {
      int top = a[a.length-1];
      a.push(top+1);
    }
  }

  function add1() public {
    if (a.length == 0) {
      a.push(1);
    } else {
      int top = a[a.length-1];
      a.push(top+1);
    }
  }
}

/// @notice invariant forall (uint i, uint j) !(0 <= i && i < j && j < a.length) || (a[i] < a[j])
contract C_sorted {
  int[] a;
  function add() public {
    if (a.length == 0) {
      a.push(0);
    } else {
      int top = a[a.length-1];
      a.push(top+1);
    }
  }
}
