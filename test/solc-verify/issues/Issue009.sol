// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract C {}

contract Issue009 {
    C m = new C();
}
