import numpy as np
std = np.load("D:\\ai_mod\\onnx-models--nvidia--Kimodo-SOMA-RP-v1.1\\stats\\global_root\\std.npy")
print(std.dtype, std.shape, std.min(), std.max())
print(std)  # 看所有值