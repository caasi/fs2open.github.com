import torch, soundfile as sf
from voxcpm import VoxCPM
O = "/dev/shm/tts-vox/out/"
m = VoxCPM.from_pretrained("openbmb/VoxCPM2", load_denoiser=False); SR = m.tts_model.sample_rate
T = "佢哋喺我後面！邊個嚟幫我甩甩佢！"
cases = {
  "a-clone": ("(speaking Cantonese, urgent, shouting)" + T, dict(reference_wav_path=O + "pilot-ref.wav")),
  "b-design-en": ("(A male fighter pilot in his thirties, deep male voice, speaking Cantonese, urgent and shouting)" + T, {}),
}
for name, (text, kw) in cases.items():
    for seed in (1, 2, 3):
        torch.manual_seed(seed)
        w = m.generate(text=text, cfg_value=2.0, inference_timesteps=10, **kw)
        sf.write(O + f"yue-pilot-{name}-s{seed}.ogg", w, SR, format="OGG", subtype="VORBIS")
