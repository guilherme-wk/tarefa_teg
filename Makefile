CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -pedantic -O2
LDLIBS  = -lm

grafo_iris: main.c grafo.c grafo.h
	$(CC) $(CFLAGS) -o $@ main.c grafo.c $(LDLIBS)

# teste automatizado: alimenta o menu com uma sequencia de opcoes
# (1 carga | 2 salvar | 3 recarregar | 5 verificar | 0 sair)
teste: grafo_iris
	printf '1\n\n2\n\ns\n3\n\n5\n0\n' | ./grafo_iris

clean:
	rm -f grafo_iris