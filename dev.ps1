#!/bin/pwsh

$ErrorActionPreference = 'Stop'

./scripts/build-client.ps1
./scripts/build-default.ps1
./scripts/run-default.ps1
