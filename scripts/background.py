# %%
import numpy as np
import matplotlib.pyplot as plt

# old_background_path = r"A:\OCT_Data\OCTcalib\SSOCTBackground.txt"
# old_background = np.loadtxt(old_background_path)

# %%
new_background_bin = (
    r"D:\Data\OCT Endometrial\20250121 background\SSOCT202501211718113DPart0.dat"
)
new_background = np.fromfile(new_background_bin, dtype=np.uint16)
new_background = new_background.reshape((new_background.size // 6144, 6144))
new_background = new_background[:10000].mean(axis=0)
np.savetxt("SSOCTBackground.txt", new_background)

# %%
