/*
 * main.c -- menu interativo para o TAD Grafo (Tarefa 1_A -- TEG).
 *
 * COMO COMPILAR (na pasta com main.c, grafo.c e grafo.h):
 *
 *     gcc -std=c99 -Wall -Wextra -O2 -o grafo_iris main.c grafo.c -lm
 *
 *   (ou apenas "make", se o make estiver instalado)
 *
 * COMO EXECUTAR (a partir da pasta do projeto, onde estao tambem
 * original_IrisDataset.csv e visualizar_grafo.py):
 *
 *     Linux / macOS / WSL :  ./grafo_iris
 *     Windows (PowerShell):  .\grafo_iris.exe
 *
 * Ao executar aparece um menu no terminal com todas as operacoes. Em
 * qualquer pergunta, apertar ENTER usa o valor padrao mostrado entre
 * colchetes. O limiar da DEN e fixo em 0,3 (enunciado, item d).
 * A opcao 8 (visualizacao 3D) requer: pip install plotly numpy
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "grafo.h"

#ifdef _WIN32
#define PYTHON "python"
#else
#define PYTHON "python3"
#endif

#define SCRIPT_PY     "visualizar_grafo.py"
#define BASE_PADRAO   "original_IrisDataset.csv"
#define GRAFO_PADRAO  "grafo_iris.csv"
#define TAM           512

typedef struct {
    Grafo *g;
    char base[TAM];    /* CSV da base Iris usado na carga primaria ("" = desconhecido) */
    char arquivo[TAM]; /* CSV de persistencia associado ao grafo ("" = nenhum)         */
    int salvo;         /* 1 = o grafo em memoria ja esta gravado em `arquivo`          */
} Estado;

/* ------------------------------------------------------------------ */
/* Entrada                                                            */
/* ------------------------------------------------------------------ */

/* Le uma linha de stdin, sem \n e sem espacos nas pontas. -1 em EOF. */
static int ler_linha(char *buf, size_t cap) {
    size_t k;
    char *ini = buf;
    if (fgets(buf, (int)cap, stdin) == NULL) return -1;
    k = strlen(buf);
    if (k > 0 && buf[k - 1] == '\n') {
        buf[--k] = '\0';
    } else {
        int c; /* linha maior que o buffer: descarta o excesso */
        while ((c = getchar()) != '\n' && c != EOF) {}
    }
    while (k > 0 && isspace((unsigned char)buf[k - 1])) buf[--k] = '\0';
    while (*ini && isspace((unsigned char)*ini)) ini++;
    if (ini != buf) memmove(buf, ini, strlen(ini) + 1);
    return 0;
}

/* Pergunta com valor padrao. Aceita caminho entre aspas (arrastar arquivo). */
static int pedir(const char *rotulo, const char *padrao, char *dst, size_t cap) {
    size_t k;
    if (padrao != NULL && padrao[0] != '\0') printf("%s [%s]: ", rotulo, padrao);
    else printf("%s: ", rotulo);
    fflush(stdout);
    if (ler_linha(dst, cap) != 0) return -1;
    k = strlen(dst);
    if (k >= 2 && dst[0] == '"' && dst[k - 1] == '"') {
        memmove(dst, dst + 1, k - 2);
        dst[k - 2] = '\0';
    }
    if (dst[0] == '\0' && padrao != NULL) snprintf(dst, cap, "%s", padrao);
    return 0;
}

static int arquivo_existe(const char *caminho) {
    FILE *f = fopen(caminho, "r");
    if (f == NULL) return 0;
    fclose(f);
    return 1;
}

/* Pergunta um vertice ("c12" ou "12"). Retorna indice 0-based ou -1. */
static int pedir_vertice(const char *rotulo, int n) {
    char buf[64], *fim;
    const char *p;
    long k;
    if (pedir(rotulo, NULL, buf, sizeof(buf)) != 0) return -1;
    p = buf;
    if (*p == 'c' || *p == 'C') p++;
    k = strtol(p, &fim, 10);
    if (p == fim || *fim != '\0' || k < 1 || k > n) {
        printf("  Vertice invalido. Use um valor de 1 a %d (ex.: c12 ou 12).\n", n);
        return -1;
    }
    return (int)k - 1;
}

/* ------------------------------------------------------------------ */
/* Operacoes do menu                                                  */
/* ------------------------------------------------------------------ */

static int tem_grafo(const Estado *e) {
    if (e->g == NULL) {
        printf("  Nenhum grafo na memoria. Use a opcao 1 (carga) ou 3 (recarregar) primeiro.\n");
        return 0;
    }
    return 1;
}

static void trocar_grafo(Estado *e, Grafo *novo) {
    grafo_destruir(e->g);
    e->g = novo;
}

static void op_carga(Estado *e) {
    char base[TAM], erro[256] = "";
    Grafo *novo;

    if (pedir("Arquivo CSV da base Iris", e->base[0] ? e->base : BASE_PADRAO, base, sizeof(base)) != 0) return;
    novo = grafo_carregar_iris_csv(base, LIMIAR_PADRAO, erro, sizeof(erro));
    if (novo == NULL) {
        printf("  Erro na carga: %s\n", erro);
        return;
    }
    trocar_grafo(e, novo);
    snprintf(e->base, sizeof(e->base), "%s", base);
    e->arquivo[0] = '\0';
    e->salvo = 0;
    printf("\n  Carga concluida (aresta se DEN <= %.1f).\n\n", LIMIAR_PADRAO);
    grafo_imprimir_resumo(e->g, stdout);
    printf("\n  Lembre-se de salvar o grafo (opcao 2).\n");
}

/* Salva o grafo em `caminho`; atualiza o estado. Retorna 0 em sucesso. */
static int salvar_em(Estado *e, const char *caminho) {
    char erro[256] = "";
    if (grafo_salvar_csv(e->g, caminho, erro, sizeof(erro)) != 0) {
        printf("  Erro ao salvar: %s\n", erro);
        return -1;
    }
    snprintf(e->arquivo, sizeof(e->arquivo), "%s", caminho);
    e->salvo = 1;
    printf("  Grafo salvo em '%s'.\n", caminho);
    return 0;
}

static int op_salvar(Estado *e) {
    char caminho[TAM], resp[16];
    if (!tem_grafo(e)) return -1;
    if (pedir("Arquivo CSV de saida", e->arquivo[0] ? e->arquivo : GRAFO_PADRAO, caminho, sizeof(caminho)) != 0) return -1;
    if (arquivo_existe(caminho)) {
        printf("  O arquivo '%s' ja existe. Sobrescrever? (s/N): ", caminho);
        fflush(stdout);
        if (ler_linha(resp, sizeof(resp)) != 0 || (resp[0] != 's' && resp[0] != 'S')) {
            printf("  Operacao cancelada.\n");
            return -1;
        }
    }
    return salvar_em(e, caminho);
}

static void op_recarregar(Estado *e) {
    char caminho[TAM], erro[256] = "";
    Grafo *novo;
    if (pedir("Arquivo CSV do grafo", e->arquivo[0] ? e->arquivo : GRAFO_PADRAO, caminho, sizeof(caminho)) != 0) return;
    novo = grafo_carregar_csv(caminho, erro, sizeof(erro));
    if (novo == NULL) {
        printf("  Erro ao recarregar: %s\n", erro);
        return;
    }
    trocar_grafo(e, novo);
    snprintf(e->arquivo, sizeof(e->arquivo), "%s", caminho);
    e->salvo = 1;
    printf("\n  Grafo recarregado (sem reler a base e sem recalcular DE/DEN).\n\n");
    grafo_imprimir_resumo(e->g, stdout);
}

static void op_consultar_vertice(const Estado *e) {
    int v, j, cont = 0, n;
    if (!tem_grafo(e)) return;
    n = grafo_num_vertices(e->g);
    v = pedir_vertice("Vertice (1 a N)", n);
    if (v < 0) return;
    printf("  c%d: grau = %d\n  Vizinhos:", v + 1, grafo_grau(e->g, v));
    for (j = 0; j < n; j++) {
        int m = grafo_multiplicidade(e->g, v, j);
        if (m <= 0) continue;
        if (cont % 10 == 0) printf("\n    ");
        if (j == v) printf("c%d(laco) ", j + 1);
        else if (m > 1) printf("c%d(x%d) ", j + 1, m);
        else printf("c%d ", j + 1);
        cont++;
    }
    if (cont == 0) printf(" (nenhum -- vertice isolado)");
    printf("\n");
}

static void op_adjacencia(const Estado *e) {
    int u, v, m, n;
    if (!tem_grafo(e)) return;
    n = grafo_num_vertices(e->g);
    u = pedir_vertice("Primeiro vertice", n);
    if (u < 0) return;
    v = pedir_vertice("Segundo vertice", n);
    if (v < 0) return;
    m = grafo_multiplicidade(e->g, u, v);
    if (m > 0) printf("  c%d e c%d SAO adjacentes (%d aresta%s).\n", u + 1, v + 1, m, m > 1 ? "s" : "");
    else printf("  c%d e c%d NAO sao adjacentes.\n", u + 1, v + 1);
}

/* Coloca `s` entre aspas apropriadas ao shell em dst. Retorna 0 se coube. */
static int citar(char *dst, size_t cap, const char *s) {
    size_t k = 0;
#ifdef _WIN32
    if (strchr(s, '"') != NULL) return -1;
    return (size_t)snprintf(dst, cap, "\"%s\"", s) < cap ? 0 : -1;
#else
    if (cap < 3) return -1;
    dst[k++] = '\'';
    for (; *s; s++) {
        if (*s == '\'') { /* ' -> '\'' */
            if (k + 4 >= cap) return -1;
            memcpy(dst + k, "'\\''", 4);
            k += 4;
        } else {
            if (k + 1 >= cap) return -1;
            dst[k++] = *s;
        }
    }
    if (k + 2 > cap) return -1;
    dst[k++] = '\'';
    dst[k] = '\0';
    return 0;
#endif
}

static void op_visualizar(Estado *e) {
    char base[TAM], qb[2 * TAM + 8], qg[2 * TAM + 8], cmd[4 * TAM + 128];
    if (!tem_grafo(e)) return;
    if (!arquivo_existe(SCRIPT_PY)) {
        printf("  Nao encontrei '%s' na pasta atual. Execute o programa a partir da pasta do projeto.\n", SCRIPT_PY);
        return;
    }
    /* o script le o grafo do CSV: garante que esta gravado e atualizado */
    if (!e->salvo || e->arquivo[0] == '\0') {
        printf("  O script Python le o grafo a partir do CSV; e preciso salva-lo antes.\n");
        if (op_salvar(e) != 0) return;
    }
    if (pedir("Arquivo CSV da base Iris (posicoes/especies)", e->base[0] ? e->base : BASE_PADRAO, base, sizeof(base)) != 0) return;
    if (citar(qb, sizeof(qb), base) != 0 || citar(qg, sizeof(qg), e->arquivo) != 0) {
        printf("  Caminho de arquivo invalido para a chamada do script.\n");
        return;
    }
    snprintf(cmd, sizeof(cmd), "%s %s --base %s --grafo %s", PYTHON, SCRIPT_PY, qb, qg);
    printf("  Abrindo a visualizacao 3D no navegador (gire com o mouse)...\n");
    if (system(cmd) != 0) {
        printf("  O script Python falhou. Verifique se o Python esta instalado e rode:\n"
               "    pip install plotly numpy\n");
    }
}

/* ------------------------------------------------------------------ */
/* Menu                                                               */
/* ------------------------------------------------------------------ */

static void mostrar_menu(const Estado *e) {
    printf("\n==================================================\n");
    printf("   TEG - Tarefa 1_A  |  Grafo da base Iris\n");
    printf("==================================================\n");
    if (e->g != NULL) {
        const GrafoInfo *in = grafo_info(e->g);
        printf("   Grafo na memoria: %d vertices, %d arestas%s\n", in->total_vertices,
               in->est.num_arestas, e->salvo ? "" : "  [nao salvo]");
    } else {
        printf("   Grafo na memoria: (nenhum)\n");
    }
    printf("--------------------------------------------------\n");
    printf("   1) Carga primaria (ler CSV da base Iris)\n");
    printf("   2) Salvar grafo em CSV\n");
    printf("   3) Recarregar grafo de CSV\n");
    printf("   4) Mostrar resumo do grafo\n");
    printf("   5) Verificar cabecalho x matriz\n");
    printf("   6) Consultar vertice (grau e vizinhos)\n");
    printf("   7) Verificar adjacencia entre dois vertices\n");
    printf("   8) Visualizar em 3D (script Python)\n");
    printf("   0) Sair\n");
    printf("--------------------------------------------------\n");
    printf("Opcao: ");
    fflush(stdout);
}

int main(void) {
    Estado e;
    char linha[64];
    int rodando = 1;

    memset(&e, 0, sizeof(e));
    while (rodando) {
        char *fim;
        long op;
        mostrar_menu(&e);
        if (ler_linha(linha, sizeof(linha)) != 0) { /* EOF (Ctrl+D / Ctrl+Z) */
            printf("\n");
            break;
        }
        op = strtol(linha, &fim, 10);
        if (linha[0] == '\0' || *fim != '\0') {
            printf("  Opcao invalida.\n");
            continue;
        }
        printf("\n");
        switch (op) {
        case 1: op_carga(&e); break;
        case 2: op_salvar(&e); break;
        case 3: op_recarregar(&e); break;
        case 4: if (tem_grafo(&e)) grafo_imprimir_resumo(e.g, stdout); break;
        case 5: if (tem_grafo(&e)) grafo_verificar(e.g, stdout); break;
        case 6: op_consultar_vertice(&e); break;
        case 7: op_adjacencia(&e); break;
        case 8: op_visualizar(&e); break;
        case 0:
            if (e.g != NULL && !e.salvo) {
                char r[16];
                printf("  O grafo atual nao foi salvo. Sair mesmo assim? (s/N): ");
                fflush(stdout);
                if (ler_linha(r, sizeof(r)) != 0 || (r[0] != 's' && r[0] != 'S')) break;
            }
            rodando = 0;
            break;
        default: printf("  Opcao invalida.\n");
        }
    }
    grafo_destruir(e.g);
    printf("Ate logo!\n");
    return 0;
}