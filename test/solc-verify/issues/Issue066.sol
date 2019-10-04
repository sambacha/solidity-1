pragma solidity >=0.5.0;

contract Issue066 {
	string b;

	function f() public {
		string memory d = b;
		b = d;
	}
}
