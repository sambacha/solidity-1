pragma solidity >=0.5.0;

contract C {
	string a;
	string b;
    string[] arr;
	function f() public view {
		string storage c = a;
		string memory d = b;
		d = string(c);
	}
}