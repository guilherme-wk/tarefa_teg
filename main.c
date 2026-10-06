/*
 * main.c -- programa de demonstracao do TAD Grafo (Tarefa 1_A -- TEG).
 *
 * Uso:
 *   ./grafo_iris carregar   <base_iris.csv> <grafo_saida.csv> [limiar]
 *       carga primaria: CSV -> DE -> DEN -> limiar -> matriz de adjacencias,
 *       imprime o resumo e persiste o grafo em CSV.
 *
 *   ./grafo_iris recarregar <grafo.csv> [--verificar]
 *       recarrega o grafo persistido (sem reler a base e sem recalcular DE/DEN)
 *       e imprime o resumo. Com --verificar confere o cabecalho com a matriz.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "grafo.h"

static void uso(const char *prog) {
    fprintf(stderr,
            "Uso:\n"
            "  %s carregar   <base_iris.csv> <grafo_saida.csv> [limiar]\n"
            "  %s recarregar <grafo.csv> [--verificar]\n",
            prog, prog);
}

int main(int argc, char **argv) {
    char erro[256] = "";
    Grafo *g;
    int status = 0;

    if (argc >= 4 && strcmp(argv[1], "carregar") == 0) {
        double limiar = LIMIAR_PADRAO;
        if (argc >= 5) {
            char *fim;
            limiar = strtod(argv[4], &fim);
            if (*fim != '\0' || limiar < 0.0 || limiar > 1.0) {
                fprintf(stderr, "erro: limiar deve estar em [0,1]\n");
                return 2;
            }
        }
        g = grafo_carregar_iris_csv(argv[2], limiar, erro, sizeof(erro));
        if (g == NULL) {
            fprintf(stderr, "erro na carga: %s\n", erro);
            return 1;
        }
        grafo_imprimir_resumo(g, stdout);
        if (grafo_salvar_csv(g, argv[3], erro, sizeof(erro)) != 0) {
            fprintf(stderr, "erro ao salvar: %s\n", erro);
            status = 1;
        } else {
            printf("\nGrafo persistido em '%s'\n", argv[3]);
        }
        grafo_destruir(g);
        return status;
    }

    if (argc >= 3 && strcmp(argv[1], "recarregar") == 0) {
        g = grafo_carregar_csv(argv[2], erro, sizeof(erro));
        if (g == NULL) {
            fprintf(stderr, "erro ao recarregar: %s\n", erro);
            return 1;
        }
        grafo_imprimir_resumo(g, stdout);
        if (argc >= 4 && strcmp(argv[3], "--verificar") == 0) {
            printf("\n");
            if (grafo_verificar(g, stdout) != 0) status = 1;
        }
        grafo_destruir(g);
        return status;
    }

    uso(argv[0]);
    return 2;
}
