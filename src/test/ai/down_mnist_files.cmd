curl http://yann.lecun.com/exdb/mnist/train-images-idx3-ubyte.gz --output ./data/train-images-idx3-ubyte.gz
curl http://yann.lecun.com/exdb/mnist/train-labels-idx1-ubyte.gz --output ./data/train-labels-idx1-ubyte.gz
curl http://yann.lecun.com/exdb/mnist/t10k-images-idx3-ubyte.gz --output ./data/t10k-images-idx3-ubyte.gz
curl http://yann.lecun.com/exdb/mnist/t10k-labels-idx1-ubyte.gz --output ./data/t10k-labels-idx1-ubyte.gz
"C:\Program Files\7-Zip\7z.exe" e %~dp0data/train-images-idx3-ubyte.gz -o%~dp0data/
"C:\Program Files\7-Zip\7z.exe" e %~dp0data/train-labels-idx1-ubyte.gz -o%~dp0data/
"C:\Program Files\7-Zip\7z.exe" e %~dp0data/t10k-images-idx3-ubyte.gz -o%~dp0data/
"C:\Program Files\7-Zip\7z.exe" e %~dp0data/t10k-labels-idx1-ubyte.gz -o%~dp0data/
