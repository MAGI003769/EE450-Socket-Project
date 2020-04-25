all: client.c aws.c serverA.c serverB.c serverC.c
		gcc -o client client.c
		gcc -o aws aws.c
		gcc -o serverA serverA.c
		gcc -o serverB serverB.c
		gcc -o serverC serverC.c -lm  #-lm is to compile math.h used for rounding

aws:
		./aws

serverA:
		./serverA

serverB:
		./serverB

serverC:
		./serverC

.PHONY: aws serverA serverB serverC
