// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract PushBeforeLocal {
    struct S{ int x; }

    S[][] ss;
    S[] to_push;

    // This is to test that if we push to the end of the first dimension of a 2D array
    // then the beginning of the second dimension does not get shifted
    constructor() {

        // Create this layout: [ [S(1)], [S(2)]]
        ss.push(to_push);
        ss.push(to_push);
        require(ss[0].length == 0); // TODO: required because elements are not set to default when resizing
        require(ss[1].length == 0);
        ss[0].push(S(1));
        ss[1].push(S(2));

        // Point to S(2)
        S storage s = ss[1][0];

        // Update to this layout: [ [S(1), S(3)], [S(2)]]
        ss[0].push(S(3));

        // Pointer should still point to S(2)
        assert(s.x == 2);
    }

    receive() external payable {
    }
}
