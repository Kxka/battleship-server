# Multiplayer Online Battleship Boardgame 

  A networked Battleship game server implemented in C using TCP sockets and I/O multiplexing.

## Features

Handles multiple players concurrently on server 

Broadcast messages to players

Error handling for client disconnections

## Message Protocol
Registration
REG alice 3 4 -

Bombing
BOMB <x> <y>

## Server Responses

WELCOME – Successful registration

INVALID – Invalid command

TAKEN – Name already in use

JOIN <name> – A new player joined

HIT <attacker> <x> <y> <victim>

MISS <attacker> <x> <y>

GG <player> – Player eliminated
