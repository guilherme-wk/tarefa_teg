#!/usr/bin/env python3
"""
visualizar_grafo.py -- visualizacao tridimensional interativa do grafo Iris
(Tarefa 1_A -- TEG, item f).

Le:
  * a base Iris original (CSV)  -> posicao 3D de cada vertice e a especie;
  * o grafo persistido pelo programa em C (grafo_iris.csv) -> arestas
    (lidas da matriz de adjacencias; o programa em C nao e reprocessado).

Cada observacao tem 4 medidas, mas um grafico 3D so tem 3 eixos. Por isso:
  --eixos pca        (padrao) projeta as 4 medidas nos 3 componentes principais
                      (PCA sobre as medidas em cm, sem padronizar -- coerente
                      com a distancia euclideana usada no grafo);
  --eixos atributos  usa 3 das 4 medidas originais, escolhidas com --attrs.

Interacao (Plotly): arrastar com o mouse gira o grafico, a roda do mouse da
zoom, shift+arrastar desloca; passar o mouse sobre um vertice mostra seus dados.

Fonte / referencias:
  Script escrito com base nos exemplos da documentacao do Plotly:
    https://plotly.com/python/3d-scatter-plots/   (go.Scatter3d, marcadores)
    https://plotly.com/python/3d-line-plots/      (go.Scatter3d, linhas)
  PCA via decomposicao SVD (numpy.linalg.svd).

Dependencias:  pip install plotly numpy

Exemplos:
  python3 visualizar_grafo.py                               # abre no navegador
  python3 visualizar_grafo.py --saida grafo_iris_3d.html --sem-abrir
  python3 visualizar_grafo.py --eixos atributos --attrs 0 2 3
"""
import argparse
import csv
import sys

import numpy as np
import plotly.graph_objects as go

NOMES_ATRIBUTOS = ["Sepal.length", "Sepal.width", "Petal.length", "Petal.width"]
CORES_ESPECIES = ["#1f77b4", "#ff7f0e", "#2ca02c", "#9467bd"]


def ler_base_iris(caminho):
    """Retorna (especies, X) -- X tem shape (n, 4). A 1a coluna e a especie."""
    especies, linhas = [], []
    with open(caminho, newline="", encoding="utf-8-sig") as f:
        for r in csv.reader(f):
            if not r or not "".join(r).strip():
                continue
            try:
                valores = [float(x) for x in r[1:5]]
            except ValueError:
                if not linhas:  # primeira linha nao numerica = cabecalho
                    continue
                raise
            especies.append(r[0].strip())
            linhas.append(valores)
    return especies, np.array(linhas, dtype=float)


def ler_grafo(caminho):
    """Le o CSV de persistencia. Retorna (info, matriz) -- info e dict de str."""
    info, matriz, na_matriz = {}, [], False
    with open(caminho, newline="", encoding="utf-8") as f:
        for r in csv.reader(f):
            if not r:
                continue
            if na_matriz:
                matriz.append([int(x) for x in r])
            elif r[0] == "matriz_adjacencias":
                na_matriz = True
            else:
                info.setdefault(r[0], r[1:])
    if not na_matriz:
        sys.exit(f"erro: '{caminho}' nao contem a matriz de adjacencias")
    return info, np.array(matriz, dtype=int)


def coordenadas_pca(X):
    """Projeta X (n,4) nos 3 primeiros componentes principais."""
    Xc = X - X.mean(axis=0)
    _, s, vt = np.linalg.svd(Xc, full_matrices=False)
    var = s**2 / np.sum(s**2)
    return Xc @ vt[:3].T, var[:3]


def montar_figura(especies, X, A, eixos, attrs):
    n = len(X)
    if eixos == "pca":
        P, var = coordenadas_pca(X)
        titulos = [f"PC{i + 1} ({100 * var[i]:.1f}%)" for i in range(3)]
    else:
        P = X[:, attrs]
        titulos = [NOMES_ATRIBUTOS[i] + " (cm)" for i in attrs]

    # --- arestas: um unico trace; None separa os segmentos ---
    iu, ju = np.nonzero(np.triu(A, k=1))
    ex, ey, ez = [], [], []
    for i, j in zip(iu, ju):
        ex += [P[i, 0], P[j, 0], None]
        ey += [P[i, 1], P[j, 1], None]
        ez += [P[i, 2], P[j, 2], None]

    fig = go.Figure()
    fig.add_trace(
        go.Scatter3d(
            x=ex, y=ey, z=ez, mode="lines", name=f"arestas ({len(iu)})",
            line=dict(color="rgba(70,70,70,0.25)", width=1), hoverinfo="skip",
        )
    )

    # --- vertices: um trace por especie (legenda clicavel) ---
    grau = A.sum(axis=1) - np.diag(A)
    for k, esp in enumerate(dict.fromkeys(especies)):
        idx = [i for i in range(n) if especies[i] == esp]
        textos = [
            f"c{i + 1} - {esp}<br>grau: {grau[i]}<br>"
            + "<br>".join(f"{NOMES_ATRIBUTOS[a]}: {X[i, a]:g}" for a in range(4))
            for i in idx
        ]
        fig.add_trace(
            go.Scatter3d(
                x=P[idx, 0], y=P[idx, 1], z=P[idx, 2], mode="markers", name=esp,
                text=textos, hoverinfo="text",
                marker=dict(size=4, color=CORES_ESPECIES[k % len(CORES_ESPECIES)],
                            line=dict(color="white", width=0.5)),
            )
        )

    fig.update_layout(
        title=f"Grafo Iris: {n} vertices, {len(iu)} arestas (DEN <= limiar)",
        scene=dict(xaxis_title=titulos[0], yaxis_title=titulos[1],
                   zaxis_title=titulos[2], aspectmode="data"),
        legend=dict(itemsizing="constant"),
        margin=dict(l=0, r=0, t=50, b=0),
    )
    return fig


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--base", default="original_IrisDataset.csv",
                    help="CSV da base Iris original (padrao: %(default)s)")
    ap.add_argument("--grafo", default="grafo_iris.csv",
                    help="CSV do grafo persistido pelo programa C (padrao: %(default)s)")
    ap.add_argument("--eixos", choices=["pca", "atributos"], default="pca")
    ap.add_argument("--attrs", type=int, nargs=3, default=[0, 2, 3],
                    metavar=("A", "B", "C"),
                    help="com --eixos atributos: 3 indices entre 0..3 "
                         "(0=Sepal.length 1=Sepal.width 2=Petal.length 3=Petal.width)")
    ap.add_argument("--saida", help="grava tambem um HTML interativo neste arquivo")
    ap.add_argument("--sem-abrir", action="store_true",
                    help="nao abre o navegador (util junto com --saida)")
    args = ap.parse_args()

    if any(a not in range(4) for a in args.attrs) or len(set(args.attrs)) != 3:
        sys.exit("erro: --attrs precisa de 3 indices distintos entre 0 e 3")

    especies, X = ler_base_iris(args.base)
    info, A = ler_grafo(args.grafo)
    if A.shape[0] != A.shape[1] or A.shape[0] != len(X):
        sys.exit(f"erro: a base tem {len(X)} observacoes, mas a matriz do grafo e {A.shape}")
    if int(info["total_vertices"][0]) != len(X):
        sys.exit("erro: total_vertices do cabecalho difere da base")

    fig = montar_figura(especies, X, A, args.eixos, args.attrs)
    if args.saida:
        fig.write_html(args.saida, include_plotlyjs="cdn")
        print(f"HTML gravado em {args.saida}")
    if not args.sem_abrir:
        fig.show()


if __name__ == "__main__":
    main()
