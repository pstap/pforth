# pforth

A forth implementation in C for learning purposes

At the time of writing, I have spent more time at a pforth prompt than any other
forth implementation. This probably shows in the code.

Originally written over a few nights in Jan 2021

## compiling
`$ make`
`$ ./pf`

## example session

```
Welcome to pforth
Type DICT<ENTER> for a DICT listing, or WORDS<ENTER> for all words

i       Addr            Word
0       0x562c9f8568f0  "+"
1       0x562c9f856910  "-"
2       0x562c9f856930  "*"
3       0x562c9f856950  "."
4       0x562c9f856970  "PUSH"
5       0x562c9f856990  "CR"
6       0x562c9f8569b0  "DUP"
7       0x562c9f8569d0  "SWAP"
8       0x562c9f8569f0  ".s"
9       0x562c9f856a10  ".d"
> : INC PUSH 1 + ;
> : SQR
COMPILE> DUP *
COMPILE> ;
> .d
i       Addr            Word
0       0x562c9f8568f0  "+"
1       0x562c9f856910  "-"
2       0x562c9f856930  "*"
3       0x562c9f856950  "."
4       0x562c9f856970  "PUSH"
5       0x562c9f856990  "CR"
6       0x562c9f8569b0  "DUP"
7       0x562c9f8569d0  "SWAP"
8       0x562c9f8569f0  ".s"
9       0x562c9f856a10  ".d"
10      0x562c9f856a30  "INC"
11      0x562c9f856a50  "SQR"
> 10 INC SQR . CR
121
> bye.
```
