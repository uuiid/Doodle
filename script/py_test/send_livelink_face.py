"""发送自定义 Live Link Face 测试数据源.

使用 PyLiveLinkFace (https://github.com/JimWest/PyLiveLinkFace) 将 61 个 ARKit
blendshape 数据按 Live Link Face 协议编码后, 通过 UDP 发送给 DoodleLiveLink
(UE 侧 Live Link Face 源), 用于验证接收 / 转发链路.

默认目标与 DoodleLiveLink 的默认配置一致:
    - 地址: 127.0.0.1
    - 端口: 14785
    - Subject 名: DoodleFace

用法:
    python send_livelink_face.py
    python send_livelink_face.py --host 127.0.0.1 --port 14785 --name DoodleFace
    python send_livelink_face.py --frames 300 --interval 0.033
"""

import argparse
import math
import socket
import time

from pylivelinkface import FaceBlendShape, PyLiveLinkFace


def animate_blendshapes(face: PyLiveLinkFace, frame: int) -> None:
    """按帧设置自定义 blendshape 动画, 便于观察数据是否流动.

    使用正弦波驱动头部旋转与眼睛、嘴巴等关键 blendshape,
    `no_filter=True` 关闭库内部的均值滤波, 让数值直接生效.
    """
    t = frame * 0.1

    # 头部旋转 (范围 -1 ~ 1)
    face.set_blendshape(FaceBlendShape.HeadPitch, math.sin(t) * 0.3, no_filter=True)
    face.set_blendshape(FaceBlendShape.HeadYaw, math.sin(t * 0.7) * 0.4, no_filter=True)
    face.set_blendshape(FaceBlendShape.HeadRoll, math.sin(t * 0.5) * 0.2, no_filter=True)

    # 眨眼 (范围 0 ~ 1), 用绝对值制造周期性眨眼
    blink = abs(math.sin(t * 2.0)) ** 4
    face.set_blendshape(FaceBlendShape.EyeBlinkLeft, blink, no_filter=True)
    face.set_blendshape(FaceBlendShape.EyeBlinkRight, blink, no_filter=True)

    # 嘴巴 (范围 0 ~ 1)
    jaw = (math.sin(t) + 1.0) * 0.5
    face.set_blendshape(FaceBlendShape.JawOpen, jaw * 0.8, no_filter=True)
    face.set_blendshape(FaceBlendShape.MouthSmileLeft, jaw * 0.5, no_filter=True)
    face.set_blendshape(FaceBlendShape.MouthSmileRight, jaw * 0.5, no_filter=True)

    # 眉毛
    face.set_blendshape(FaceBlendShape.BrowInnerUp, jaw * 0.6, no_filter=True)


def main() -> None:
    parser = argparse.ArgumentParser(description="发送自定义 Live Link Face 测试数据源")
    parser.add_argument("--host", default="127.0.0.1", help="目标地址 (默认 127.0.0.1)")
    parser.add_argument("--port", type=int, default=14785, help="目标端口 (默认 14785, 对应 DoodleLiveLink)")
    parser.add_argument("--name", default="DoodleFace", help="Subject 名 (默认 DoodleFace)")
    parser.add_argument("--fps", type=int, default=60, help="发送帧率 (默认 60)")
    parser.add_argument("--frames", type=int, default=0, help="发送帧数, 0 表示持续发送 (默认 0)")
    parser.add_argument("--interval", type=float, default=None, help="发送间隔秒数, 默认按 fps 计算")
    args = parser.parse_args()

    interval = args.interval if args.interval is not None else 1.0 / args.fps

    face = PyLiveLinkFace(name=args.name, fps=args.fps)
    print(f"Live Link Face 测试源: name={face.name}, uuid={face.uuid}, fps={face.fps}")
    print(f"目标: {args.host}:{args.port}, 发送间隔 {interval:.3f}s")

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.connect((args.host, args.port))

    frame = 0
    try:
        while args.frames == 0 or frame < args.frames:
            animate_blendshapes(face, frame)
            sock.sendall(face.encode())
            frame += 1
            if frame % 60 == 0:
                print(f"已发送 {frame} 帧")
            time.sleep(interval)
    except KeyboardInterrupt:
        print("\n停止发送")
    finally:
        sock.close()


if __name__ == "__main__":
    main()
