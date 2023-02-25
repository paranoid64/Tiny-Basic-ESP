# Tiny Basic ESP

This is a Fork of https://github.com/fg1998/ESP32-Basic-vga

There is already a further development, but unfortunately the code is not public anymore. And it lacks some useful basic functions to really program with it.

With this fork I have now added some functions and hope for participation of others to get a really good basic out of it. New is for example already:

OLD: **RND(8)** can now also be surrounded with two were:

**RND(4,8)**
Generates a random number. If 2 values are specified, a number between the 2 values is generated.

```
10 FOR I=0 TO 3
20 PRINT RND(20,25)
30 NEXT I
```

Result:
```
22
23
21
24
```
 
**Scroll**
moves the screen one line down. 


**CHR 35**
Return ASCII Char:

```
dec: 	32	33	34	35	36	37	38	39	40	41	42	43	44	45	46	47
 
byte: 	 	!	"	#	$	%	&	'	(	)	*	+	,	-	.	/

 
dec: 	48	49	50	51	52	53	54	55	56	57	58	59	60	61	62	63

byte: 	0	1	2	3	4	5	6	7	8	9	:	;	<	=	>	?


dec: 	64	65	66	67	68	69	70	71	72	73	74	75	76	77	78	79

byte: 	@	A	B	C	D	E	F	G	H	I	J	K	L	M	N	O



dec: 	80	81	82	83	84	85	86	87	88	89	90	91	92	93	94	95
 
byte: 	P	Q	R	S	T	U	V	W	X	Y	Z	[	\	]	^	_
 

dec: 	96	97	98	99	100	101	102	103	104	105	106	107	108	109	110	111
 
byte: 	`	a	b	c	d	e	f	g	h	i	j	k	l	m	n	o

 
dec: 	112	113	114	115	116	117	118	119	120	121	122	123	124 125	126
 
byte: 	p	q	r	s	t	u	v	w	x	y	z	{	|	}	~
```
 

<<<<<<< HEAD
**TAB 3**
=======
**TAB <int>**
>>>>>>> e11efa8317db9a4c6737bc603025b9db678139fa

This function influences the output so that positioning of the cursor to the desired column is achieved.

```
10 FOR I=0 TO 3
20 TAB I
30 PRINT "*"
40 NEXT I
```

Result:
```
*
 *
  *
   *
```

**MIN(10,20)**
compares 2 values and return the lowest number

```
10 A=10:B=20
20 A=MIN(A,B)
30 PRINT A
```

Result:
 
10

<<<<<<< HEAD
**MAX(10,20)**
=======
**MAX(<INT>,<INT>)**
>>>>>>> e11efa8317db9a4c6737bc603025b9db678139fa
 
compares 2 values and return the highest  number

```
10 A=10:B=20
20 A=MAX(A,B)
30 PRINT A
```

Result:
 
20

<<<<<<< HEAD
**LOG(65)**
=======
**LOG(<INT x>)**
>>>>>>> e11efa8317db9a4c6737bc603025b9db678139fa

returns the natural logarithm of x

```
10 A=65
20 PRINT LN(A)
```

Result:
 
4

<<<<<<< HEAD
**LOG(65, 2)**
=======
**LOG(<INT x>, <INT n>)**
>>>>>>> e11efa8317db9a4c6737bc603025b9db678139fa
 
returns the logarithm of x in base n

```
10 A=65:B=2
20 PRINT LOG (A,B)
```

Result:
6


**POW(2,3)**

```
10 A=2:B=3
20 PRINT POW(A,B)
```

Result:
 
8

<<<<<<< HEAD
**SGN(12)**
=======
**SGN(<int>)**
>>>>>>> e11efa8317db9a4c6737bc603025b9db678139fa

The sign function result is -1, 0 or +1 according as the argument is negative, zero or positive

```
10 PRINT SGN(12)
20 PRINT SGN(0)
30 PRINT SGN(-12)
```

Result:
```
1
0
-1
```
