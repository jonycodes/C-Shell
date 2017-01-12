myshell: myshell.c
	cc -o shell myshell.c

clean:
	rm -f myshell *.o core *~
