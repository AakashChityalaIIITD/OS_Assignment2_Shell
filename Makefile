all:
	gcc MyShell.c -lreadline -o shell
	./shell
clean:
	-@rm shell
