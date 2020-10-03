// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Issue066 {
	string b;

	function f() public {
		string memory d = b;
		b = d;
	}
}
