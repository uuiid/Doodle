"""Export TwostageDenoiser sub-models (root_model, body_model) to ONNX.

Usage:
    python script/export_denoiser_onnx.py ^
        --checkpoint D:\ai_mod\Kimodo-SOMA-RP-v1\model.safetensors ^
        --config   D:\ai_mod\Kimodo-SOMA-RP-v1\config.yaml ^
        --output   D:\ai_mod\kimodo_onnx\denoiser
"""

import argparse
import json
from pathlib import Path

import torch
import torch.nn as nn
import yaml

from kimodo.model.backbone import TransformerEncoderBlock
from kimodo.model.twostage_denoiser import TwostageDenoiser
from kimodo.motion_rep import KimodoMotionRep
from kimodo.skeleton import SOMASkeleton30


def _build_motion_rep(skeleton, fps=30):
    """Create a minimal motion_rep for dimension inference."""
    return KimodoMotionRep(skeleton=skeleton, fps=fps)


def main():
    parser = argparse.ArgumentParser(description="Export denoiser ONNX models")
    parser.add_argument("--checkpoint", required=True, help="Path to model.safetensors or .ckpt")
    parser.add_argument("--config", required=True, help="Path to Hydra config.yaml")
    parser.add_argument("--output", required=True, help="Output directory for ONNX files")
    parser.add_argument("--opset", type=int, default=17, help="ONNX opset version")
    args = parser.parse_args()

    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    # ── 1. Load config ────────────────────────────────────────────────────
    with open(args.config) as f:
        cfg = yaml.safe_load(f)

    # ── 2. Build motion_rep & skeleton ────────────────────────────────────
    #    (Modify skeleton/fps to match your actual model config)
    skeleton = SOMASkeleton30()
    motion_rep = _build_motion_rep(skeleton, fps=30)

    # ── 3. Instantiate TwostageDenoiser ───────────────────────────────────
    #    The **kwargs come from the config's backbone section
    backbone_cfg = cfg["denoiser"]["backbone"]
    denoiser = TwostageDenoiser(
        motion_rep=motion_rep,
        motion_mask_mode=backbone_cfg.get("motion_mask_mode", "none"),
        ckpt_path=args.checkpoint,
        **backbone_cfg,
    )
    denoiser.eval()

    root_model: nn.Module = denoiser.root_model
    body_model: nn.Module = denoiser.body_model

    device = next(denoiser.parameters()).device

    # ── 4. Shared dummy inputs ────────────────────────────────────────────
    B = 1
    T = 64
    L = root_model.num_text_tokens
    llm_dim = root_model.llm_shape[-1]

    x_pad_mask = torch.ones(B, T, dtype=torch.bool, device=device)
    text_feat = torch.randn(B, L, llm_dim, device=device)
    text_pad_mask = torch.ones(B, L, dtype=torch.bool, device=device)
    timesteps = torch.zeros(B, dtype=torch.long, device=device)
    first_heading = torch.zeros(B, device=device)

    dynamic_axes = {
        "x":                   {0: "batch", 1: "frames"},
        "x_pad_mask":          {0: "batch", 1: "frames"},
        "text_feat":           {0: "batch"},
        "text_pad_mask":       {0: "batch"},
        "timesteps":           {0: "batch"},
        "first_heading_angle": {0: "batch"},
        "output":              {0: "batch", 1: "frames"},
    }

    # ── 5. Export root_model ──────────────────────────────────────────────
    print(f"Exporting root_model:  input_dim={root_model.input_dim}, "
          f"output_dim={root_model.output_dim}")
    x_root = torch.randn(B, T, root_model.input_dim, device=device)
    torch.onnx.export(
        root_model,
        (x_root, x_pad_mask, text_feat, text_pad_mask, timesteps, first_heading),
        str(output_dir / "root_model.onnx"),
        opset_version=args.opset,
        do_constant_folding=True,
        input_names=["x", "x_pad_mask", "text_feat", "text_pad_mask",
                     "timesteps", "first_heading_angle"],
        output_names=["output"],
        dynamic_axes=dynamic_axes,
    )
    print("  ✓")

    # ── 6. Export body_model ──────────────────────────────────────────────
    print(f"Exporting body_model:  input_dim={body_model.input_dim}, "
          f"output_dim={body_model.output_dim}")
    x_body = torch.randn(B, T, body_model.input_dim, device=device)
    torch.onnx.export(
        body_model,
        (x_body, x_pad_mask, text_feat, text_pad_mask, timesteps, first_heading),
        str(output_dir / "body_model.onnx"),
        opset_version=args.opset,
        do_constant_folding=True,
        input_names=["x", "x_pad_mask", "text_feat", "text_pad_mask",
                     "timesteps", "first_heading_angle"],
        output_names=["output"],
        dynamic_axes=dynamic_axes,
    )
    print("  ✓")

    # ── 7. Save metadata ──────────────────────────────────────────────────
    meta = {
        "motion_rep_dim":     motion_rep.motion_rep_dim,
        "global_root_dim":    motion_rep.global_root_dim,
        "local_root_dim":     motion_rep.local_root_dim,
        "body_dim":           motion_rep.body_dim,
        "llm_dim":            int(llm_dim),
        "num_text_tokens":    int(root_model.num_text_tokens),
        "fps":                int(motion_rep.fps),
        "input_first_heading_angle": bool(root_model.input_first_heading_angle),
        "motion_mask_mode":   denoiser.motion_mask_mode,
        "root_input_dim":     int(root_model.input_dim),
        "root_output_dim":    int(root_model.output_dim),
        "body_input_dim":     int(body_model.input_dim),
        "body_output_dim":    int(body_model.output_dim),
    }
    with open(str(output_dir / "model_meta.json"), "w") as f:
        json.dump(meta, f, indent=2)
    print(f"  ✓ model_meta.json")

    print(f"\nDone. Files in {output_dir}:")
    for f in output_dir.iterdir():
        print(f"  {f.name}")


if __name__ == "__main__":
    main()
