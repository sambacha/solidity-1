// SPDX-License-Identifier: GPL-3.0
pragma solidity >=0.7.0;

contract Starter
{
    enum StateType { GameProvisioned, Pingponging, GameFinished}

    StateType public State;

    string public PingPongGameName;
    address public GameStarter;
    address public GamePlayer;
    int public PingPongTimes;

    constructor (string memory gameName) {
        PingPongGameName = gameName;
        GameStarter = msg.sender;

        GamePlayer = address(new Player(PingPongGameName));

        State = StateType.GameProvisioned;
    }

    function StartPingPong(int pingPongTimes) public
    {
        PingPongTimes = pingPongTimes;

        Player player = Player(GamePlayer);
        State = StateType.Pingponging;

        player.Ping(pingPongTimes);
    }

    function Pong(int _currentPingPongTimes) public
    {
        int currentPingPongTimes = _currentPingPongTimes;
        currentPingPongTimes = currentPingPongTimes - 1;

        Player player = Player(GamePlayer);
        if(currentPingPongTimes > 0)
        {
            State = StateType.Pingponging;
            player.Ping(currentPingPongTimes);
        }
        else
        {
            State = StateType.GameFinished;
            player.FinishGame();
        }
    }

    function FinishGame() public
    {
        State = StateType.GameFinished;
    }
}

contract Player
{
    enum StateType {PingpongPlayerCreated, PingPonging, GameFinished}

    StateType public State;

    address public GameStarter;
    string public PingPongGameName;

    constructor (string memory pingPongGameName) {
        GameStarter = msg.sender;
        PingPongGameName = pingPongGameName;

        State = StateType.PingpongPlayerCreated;
    }

    function Ping(int _currentPingPongTimes) public
    {
        int currentPingPongTimes = _currentPingPongTimes;
        currentPingPongTimes = currentPingPongTimes - 1;

        Starter starter = Starter(msg.sender);
        if(currentPingPongTimes > 0)
        {
            State = StateType.PingPonging;
            starter.Pong(currentPingPongTimes);
        }
        else
        {
            State = StateType.GameFinished;
            starter.FinishGame();
        }
    }

    function FinishGame() public
    {
        State = StateType.GameFinished;
    }
}
