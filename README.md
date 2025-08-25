# Fairy-Stockfish

> [!NOTE]
> This is a fork of [Original Repo](https://github.com/fairy-stockfish/Fairy-Stockfish/).
> This is a partial README just for the changes in this fork. Please refer to the [Original README.md](README-orig.md).


## Overview

The idea of this fork is to provide a small extension to UCI for user interaction without a client:
* Show moves in SAN notation instead of long notation.
* Allow entering incremental SAN moves instead of resetting the position from scratch.
* Easier game navigation.

Extra commands added:

| Command                            | Description                                 |
|------------------------------------|---------------------------------------------|
| setoption name UCI\_SAN value true | show lines and best move using SAN notation |
| move san1 [[san2] ... ]            | play incremental moves                      |
| norm san1 [[san2] ... ]            | return normalized form of moves             |
| cm                                 | show candidate moves                        |
| back                               | retract the last move                       |
| reset                              | reset to the starting position              |


# Sample position

```
 uci
> ...
 setoption name UCI_SAN value true
 setoption name UCI_Variant value atomic
 setoption name MultiPV value 3

 position fen 4r3/2pk4/5B2/2PP4/8/4b3/8/3K4 w - - 0 1
 d

> +---+---+---+---+---+---+---+---+
> |   |   |   |   | r |   |   |   |8
> +---+---+---+---+---+---+---+---+
> |   |   | p | k |   |   |   |   |7
> +---+---+---+---+---+---+---+---+
> |   |   |   |   |   | B |   |   |6
> +---+---+---+---+---+---+---+---+
> |   |   | P | P |   |   |   |   |5
> +---+---+---+---+---+---+---+---+
> |   |   |   |   |   |   |   |   |4
> +---+---+---+---+---+---+---+---+
> |   |   |   |   | b |   |   |   |3
> +---+---+---+---+---+---+---+---+
> |   |   |   |   |   |   |   |   |2
> +---+---+---+---+---+---+---+---+
> |   |   |   | K |   |   |   |   |1 *
> +---+---+---+---+---+---+---+---+
>   a   b   c   d   e   f   g   h

# Sample interaction with the commands: cm, move, back and reset

 cm
> cm: c6 d6 Ba1 Bb2 Bc3 Bd4 Bh4 Be5 Bg5 Be7 Bg7 Bd8 Bh8 Ke2 Ke1 Kc2

 go movetime 2000
> ...

 move 1.Kc2 Bc1
> move ok

 back
 reset
```
