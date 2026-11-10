# Spike: emotions with Qwen3-TTS. VoiceDesign 1.7B (voice + emotion from text) and CustomVoice 0.6B (preset speaker + emotion instruct).
import sys, time, torch, soundfile as sf
from qwen_tts import Qwen3TTSModel

OUT = "/dev/shm/tts-spike/out/"
PILOT = "Male fighter pilot, about 30, mid-range voice, heard over a military radio"
LINES = [
    ("calm",   "calm and steady, reading a mission briefing",
     "阿爾法一號，前往跳躍點，等候進一步命令。", "Alpha 1, proceed to the jump node and await further orders."),
    ("urgent", "urgent and loud, shouting in the middle of a dogfight",
     "他們在我後面！誰來幫我把他甩掉！", "They're on my six! Somebody get him off me!"),
    ("fear",   "terrified, voice shaking, breathing fast",
     "護盾沒了……引擎起火了，我撐不住了！", "Shields are down... engines on fire, I can't hold it!"),
    ("relief", "relieved and happy, laughing a little after winning",
     "目標摧毀！好樣的，大家都活著回來了。", "Target destroyed! Good work, everyone made it back."),
]

def save(name, wavs, sr):
    sf.write(OUT + name + ".ogg", wavs[0], sr, format="OGG", subtype="VORBIS")

which = sys.argv[1]
t0 = time.time()
if which == "design":
    m = Qwen3TTSModel.from_pretrained("Qwen/Qwen3-TTS-12Hz-1.7B-VoiceDesign", device_map="cuda:0", dtype=torch.bfloat16)
    for tag, mood, zh, en in LINES:
        for lang, text in (("Chinese", zh), ("English", en)):
            w, sr = m.generate_voice_design(text=text, language=lang, instruct=f"{PILOT}. Tone: {mood}.")
            save(f"design-{tag}-{lang[:2].lower()}", w, sr)
else:
    m = Qwen3TTSModel.from_pretrained("Qwen/Qwen3-TTS-12Hz-0.6B-CustomVoice", device_map="cuda:0", dtype=torch.bfloat16)
    print("speakers:", m.get_supported_speakers())
    spk = sys.argv[2]
    for tag, mood, zh, en in LINES:
        for lang, text in (("Chinese", zh), ("English", en)):
            w, sr = m.generate_custom_voice(text=text, language=lang, speaker=spk, instruct=mood)
            save(f"custom-{spk}-{tag}-{lang[:2].lower()}", w, sr)
print("seconds:", round(time.time() - t0, 1), "max VRAM GB:", round(torch.cuda.max_memory_allocated() / 2**30, 2))
