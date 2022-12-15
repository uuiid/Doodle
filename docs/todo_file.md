大致匹配fmt字符串的正则表达式
```regexp
\{(\w+)(:[^\{\}]?[<>\^]?[\+-]?#?0?(\d+|(\{\w+\}))?\.?(\d+|(\{\w+\}))?L?[\w]?)\}
\{(\w+)(:[^\{\}]?[<>\^]?[\+-]?#?0?(?:\d+|(?:\{\w+\}))?\.?(?:\d+|(?:\{\w+\}))?L?[\w]?)\}
```


- 先安装 py torch
- pip3 install --force-reinstall --no-cache-dir torch torchvision torchaudio --extra-index-url https://download.pytorch.org/whl/cu116 --proxy 127.0.0.1:10808
- pip3 install --force-reinstall torch torchvision torchaudio --extra-index-url https://download.pytorch.org/whl/cu116 --proxy 127.0.0.1:10809