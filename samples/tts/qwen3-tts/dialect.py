import torch, soundfile as sf
from qwen_tts import Qwen3TTSModel
m = Qwen3TTSModel.from_pretrained("Qwen/Qwen3-TTS-12Hz-1.7B-VoiceDesign", device_map="cuda:0", dtype=torch.bfloat16)
print("languages:", m.get_supported_languages())
EN = "Male fighter pilot, about 30, mid-range voice, heard over a military radio. Tone: calm and steady, reading a mission briefing."
EN_M = EN + " Speaks standard Mandarin Chinese, not Cantonese."
ZH_M = "三十歲左右的男性戰機飛行員，中音，透過軍用無線電說話，語氣冷靜沉穩，像在念任務簡報。說標準普通話。"
TRAD = "阿爾法一號，前往跳躍點，等候進一步命令。"
SIMP = "阿尔法一号，前往跳跃点，等候进一步命令。"
cases = {
    "a-trad-en": (TRAD, EN), "b-simp-en": (SIMP, EN),
    "c-trad-enMandarin": (TRAD, EN_M), "d-trad-zhMandarin": (TRAD, ZH_M),
}
for name, (text, ins) in cases.items():
    for seed in (1, 2):
        torch.manual_seed(seed)
        w, sr = m.generate_voice_design(text=text, language="Chinese", instruct=ins)
        sf.write(f"out/dialect-{name}-s{seed}.ogg", w[0], sr, format="OGG", subtype="VORBIS")
