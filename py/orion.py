import pandas as pd
import anndata as ad
import scimap as sm
import scanpy as sc
import seaborn as sns
import pathlib
import numpy as np
import squidpy as sq
sns.set(color_codes=True)
import matplotlib.pyplot as plt

import time
import memory_profiler

plt.ion()

def print_keys(d, indent=0):
    for key, value in d.items():
        print(" " * indent + str(key))
        if isinstance(value, dict):
            print_keys(value, indent + 2)

homepath = "~/Sorger/orion"
csvpath = pathlib.Path(homepath, "processed_40_orion","CRC01_quantification.round.csv.gz")
adata = sm.pp.mcmicro_to_scimap(csvpath.as_posix())

# plot highest expression gene
sc.pl.highest_expr_genes(adata, n_top=20)

# PCA
sc.tl.pca(adata, svd_solver='arpack') # peform PCA
sc.pl.pca(adata, color='Pan-CK') # scatter plot in the PCA coordinates
sc.pl.pca_variance_ratio(adata) # PCs to the total variance in the data

# subset for speed during testing
small = adata[:500000,]

small.obs['X_centroid'] = [1.1,2,3,4,5]
small.obs['Y_centroid'] = [1,2,3,4,5]

# plot it
# Select only the points within the specified x and y coordinate range
x_min, x_max = 0, 6
y_min, y_max = 0, 6
x_filtered = [x_i for x_i, y_i in zip(small.obs['X_centroid'], small.obs['Y_centroid']) if x_min <= x_i <= x_max and y_min <= y_i <= y_max]
y_filtered = [y_i for x_i, y_i in zip(small.obs['X_centroid'], small.obs['Y_centroid']) if x_min <= x_i <= x_max and y_min <= y_i <= y_max]

plt.scatter(x_filtered, y_filtered, s=25, color='red')
plt.show()

# Computing the neighborhood graph in marker space
sns.heatmap(nn2, cmap='Reds')
plt.show()

small.obsm['spatial'] = small.obs[['X_centroid','Y_centroid']]
adata.obsm['spatial'] = adata.obs[['X_centroid','Y_centroid']]

start_time = time.time()
sq.gr.spatial_neighbors(small, radius=(0, 500), coord_type='generic', delaunay=False)
print("--- %s seconds ---" % (time.time() - start_time))

sq.gr.spatial_neighbors(small, coord_type='generic', delaunay=True)

small.obsp['spatial_connectivities'].toarray()
small.obsp['spatial_distances'].toarray()

_, idx = small.obsp["spatial_connectivities"][0, :].nonzero()
idx = np.append(idx, 0)
sq.pl.spatial_scatter(small[:, :],
                      connectivity_key="spatial_connectivities",
                      img=False, na_color="lightgrey")

# compoute
sm.tl.spatial_distance(small, x_coordinate='X_centroid', y_coordinate='Y_centroid',
                 z_coordinate=None, 
                               phenotype='phenotype', 
                               subset=None, 
                               imageid='imageid', 
                               label='spatial_distance')

sc.tl.umap(adata) # Build a UMAP to visualize the neighbourhood graph

# Plot the UMAP
sc.pl.umap(adata, color=['CD3D', 'CD20', 'CD163'], cmap= 'vlag', use_raw=False, s=30)

sc.tl.leiden(adata, resolution = 1) # Clustering the neighborhood graph

dt = np.dtype([('density', np.int32)])





## TOY
adata = sq.datasets.visium_fluo_adata()
