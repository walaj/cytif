print('hello world')

import pandas as pd
import anndata as ad
import scimap as sm
import scanpy as sc
import seaborn as sns
import pathlib
from matplotlib import pyplot as plt
import squidpy as sq
sns.set(color_codes=True)

homepath = "~/Sorger/orion"
csvpath = pathlib.Path(homepath, "processed_40_orion","CRC01_quantification.round.csv.gz")
adata = sm.pp.mcmicro_to_scimap(csvpath.as_posix())

small = subset_adata = adata[:100000, :]

sc.pl.highest_expr_genes(adata, n_top=20)

# PCA
sc.tl.pca(adata, svd_solver='arpack') # peform PCA
sc.pl.pca(adata, color='Pan-CK') # scatter plot in the PCA coordinates
sc.pl.pca_variance_ratio(adata) # PCs to the total variance in the data

sc.pp.neighbors(small, n_neighbors=30, n_pcs=10) # Computing the neighborhood graph

sc.tl.umap(adata) # Build a UMAP to visualize the neighbourhood graph

# Plot the UMAP
sc.pl.umap(adata, color=['CD3D', 'CD20', 'CD163'], cmap= 'vlag', use_raw=False, s=30)

sc.tl.leiden(r.adata, resolution = 1) # Clustering the neighborhood graph

dt = np.dtype([('density', np.int32)])

x = np.array([(393,), (337,), (256,)


## Napari
import napari

v = napari.Viewer()
v.scale_bar.visible = True
v.scale_bar.unit = "um"              
