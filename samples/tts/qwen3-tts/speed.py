import torch, soundfile as sf
from qwen_tts import Qwen3TTSModel
m = Qwen3TTSModel.from_pretrained("Qwen/Qwen3-TTS-12Hz-0.6B-CustomVoice", device_map="cuda:0", dtype=torch.bfloat16)
L = [("calm","calm and steady, reading a mission briefing","阿尔法一号，前往跳跃点，等候进一步命令。"),
     ("urgent","urgent and loud, shouting in the middle of a dogfight","他们在我后面！谁来帮我把他甩掉！")]
for tag, mood, text in L:
    for sp, extra in (("fast", ", speaking fast"), ("veryfast", ", speaking very fast, rapid radio chatter")):
        w, sr = m.generate_custom_voice(text=text, language="Chinese", speaker="ryan", instruct=mood + extra)
        f = f"out/speed-ryan-{tag}-{sp}.ogg"; sf.write(f, w[0], sr, format="OGG", subtype="VORBIS")
