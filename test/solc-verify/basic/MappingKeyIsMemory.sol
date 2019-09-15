pragma solidity ^0.5.0;

contract MappingKeyIsMemory {

    mapping(bytes=>int) m;

    bytes b1;
    bytes b2;

    function() external payable {
        m[b1] = 2;
        m[b2] = 2;
        assert(m[b1] == m[b2]);
    }
}
