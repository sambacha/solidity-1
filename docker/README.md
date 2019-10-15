# Docker instructions

This Dockerfile can help users quickly start working with it. The included runsv.sh script lets users run the containerized solc-verify on programs residing on the host.

To build the Dockerfile, run:

```
docker build -t solc-verify -f Dockerfile-solcverify.src .
```
